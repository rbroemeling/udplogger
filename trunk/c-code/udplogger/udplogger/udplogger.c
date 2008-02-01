#include <fcntl.h>
#include <getopt.h>
#include <inttypes.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <unistd.h>
#include <zlib.h>
#include "beacon.h"
#include "udplogger.h"
#include "trim.h"

#define __DEBUG__

// Parameter: the compression level to use when compressing log data.
uint8_t compress_level = DEFAULT_COMPRESS_LEVEL;

// Parameter: the maximum age of a log target before it is expired.
uintmax_t maximum_target_age = DEFAULT_MAXIMUM_TARGET_AGE;

// Parameter: the port on which to listen for beacons.
uint16_t listen_port = DEFAULT_LISTEN_PORT;

// Global Variable: a singly-linked list of the log targets that we should report to.
struct log_target *targets = NULL;


int main (int argc, char **argv)
{
	int data_length = 0;
	int fd = 0;
	unsigned char input_buffer[INPUT_BUFFER_SIZE];
	unsigned long input_buffer_length = 0;
	LOG_SERIAL_T log_serial = 0;
	char output_buffer[LOG_PACKET_SIZE];
	LOGGER_PID_T pid = 0;
	fd_set all_set, read_set;
	int result = 0;
	char *tmp;

	result = arguments_parse(argc, argv);
	if (result <= 0)
	{
		return result;
	}

#ifdef __DEBUG__
	printf("udplogger debug: parameter compress_level = '%u'\n", compress_level);
	printf("udplogger debug: parameter minimum_target_age = '%lu'\n", maximum_target_age);
	printf("udplogger debug: parameter listen_port = '%u'\n", listen_port);
#endif

	// Set up our socket, which will be used both to listen for beacons and to
	// send logging data.
	fd = bind_socket(listen_port);
	if (! fd)
	{
		fprintf(stderr, "could not setup socket\n");
		return -1;
	}
	
	bzero(input_buffer, INPUT_BUFFER_SIZE * sizeof(char));
	pid = (LOGGER_PID_T)getpid();
	
	FD_ZERO(&all_set);
	FD_SET(STDIN_FILENO, &all_set);
	FD_SET(fd, &all_set);
	
	for ( ; ; ; )
	{
		read_set = all_set;
		result = select(2, read_set, NULL, NULL, NULL);
		if (result < 0)
		{
			perror("select");
			return -1;
		}
		if (result > 0)
		{
			if (FD_ISSET(STDIN_FILENO, &read_set))
			{
				input_buffer = read_line(STDIN_FILENO, INPUT_BUFFER_SIZE);
				if (targets)
				{
					input_buffer_length = trim(input_buffer);
					log_serial++;
					tmp = output_buffer;
		
					memcpy(tmp, &pid, sizeof(pid));
					tmp += sizeof(pid);
		
					memcpy(tmp, &log_serial, sizeof(log_serial));
					tmp += sizeof(log_serial);
		
					data_length = sizeof(output_buffer) - sizeof(pid) - sizeof(log_serial);
					result = compress2((Bytef *)tmp, (uLongf *)&data_length, input_buffer, input_buffer_length + 1, compress_level);
					if (result == Z_OK)
					{
						send_targets(targets, fd, output_buffer, LOG_PACKET_SIZE, 0);
					}
				}
			}
			if (FD_ISSET(fd, &read_set))
			{
				targets = receive_beacon(targets, fd);
			}
		}
		targets = expire_log_targets(targets, maximum_target_age);
	}

	return 0;
}
	

int arguments_parse(int argc, char **argv)
{
	while (1)
	{
		static struct option long_options[] =
		{
			{"compress", required_argument, 0, 'c'},
			{"help", no_argument, 0, 'h'},
			{"listen", required_argument, 0, 'l'},
			{"max_target_age", required_argument, 0, 'm'},
			{0, 0, 0, 0}
		};
		int option_index = 0;
		uintmax_t uint_tmp;

		int i = getopt_long(argc, argv, "c:hl:m:", long_options, &option_index);
		if (i == -1)
		{
			break;
		}
		switch (i)
		{
			case 'c':
				uint_tmp = strtoumax(optarg, 0, 10);
				if (! uint_tmp || uint_tmp == UINT_MAX)
				{
					fprintf(stderr, "invalid compress level argument '%s'\n", optarg);
					return -1;
				}
				if (uint_tmp >= 0 && uint_tmp <= 9)
				{
					compress_level = uint_tmp;
				}
				else
				{
					fprintf(stderr, "compression level %lu is out of range (0-9)\n", uint_tmp);
				}
				break;
			case 'h':
				arguments_show_usage();
				return 0;
			case 'l':
				uint_tmp = strtoumax(optarg, 0, 10);
				if (! uint_tmp || uint_tmp == UINT_MAX)
				{
					fprintf(stderr, "invalid listen port argument '%s'\n", optarg);
					return -1;
				}
				if (uint_tmp > 0 && uint_tmp <= 0xFFFF)
				{
					listen_port = (uint16_t)uint_tmp;
				}
				else
				{
					fprintf(stderr, "listen argument %lu is out of port range (1-65535)\n", uint_tmp);
					return -1;
				}
				break;
			case 'm':
				uint_tmp = strtoumax(optarg, 0, 10);
				if (! uint_tmp || uint_tmp == UINT_MAX)
				{
					fprintf(stderr, "invalid maximum target age argument '%s'\n", optarg);
					return -1;
				}
				maximum_target_age = uint_tmp;
				break;
		}
	}
	
	return 1;
}


void arguments_show_usage()
{
	printf("Usage: udplogger [OPTIONS]\n");
	printf("\n");
	printf("  -c, --compress <level>     gzip compression level to use (0-9, default %u)\n", DEFAULT_COMPRESS_LEVEL);
	printf("  -h, --help                 display this help and exit\n");
	printf("  -l, --listen <port>        listen for beacons on the given port (default %u)\n", DEFAULT_LISTEN_PORT);
	printf("  -m, --max_target_age <age> expire log targets after <age> seconds (default %lu)\n", DEFAULT_MAXIMUM_TARGET_AGE);
	printf("\n");
}


int bind_socket(uint16_t listen_port)
{
	int fd;
	int result;
	unsigned int yes = 1;
	struct sockaddr_in listen_addr;
	
	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0)
	{
		perror("socket()");
		return fd;
	}
	
	result = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
	if (result < 0)
	{
		perror("setsockopt(SOL_SOCKET, SO_REUSEADDR)");
		return result;
	}

	result = fcntl(fd, F_SETFL, O_NONBLOCK);
	if (result == -1)
	{
		perror("fcntl(F_SETFL, O_NONBLOCK)");
		return result;
	}

	bzero(&listen_addr, sizeof(listen_addr));
	listen_addr.sin_family = AF_INET;
	listen_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	listen_addr.sin_port = htons(listen_port);

	result = bind(fd, (struct sockaddr *) &listen_addr, sizeof(listen_addr));
	if (result < 0)
	{
		perror("bind()");
		return result;
	}
	
	return fd;
}
