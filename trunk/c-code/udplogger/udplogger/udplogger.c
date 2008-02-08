#include <fcntl.h>
#include <getopt.h>
#include <inttypes.h>
#include <limits.h>
#include <pthread.h>
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


int main (int argc, char **argv)
{
	struct log_target_t *current = NULL;
	int data_length = 0;
	int fd = 0;
	unsigned char input_buffer[INPUT_BUFFER_SIZE];
	unsigned long input_line_length = 0;
	LOG_SERIAL_T log_serial = 0;
	char output_buffer[LOG_PACKET_SIZE];
	LOGGER_PID_T pid = 0;
	int result = 0;
	char *tmp;

	result = arguments_parse(argc, argv);
	if (result <= 0)
	{
		return result;
	}

#ifdef __DEBUG__
	printf("udplogger debug: parameter compress_level = '%u'\n", conf.compress_level);
	printf("udplogger debug: parameter listen_port = '%u'\n", conf.listen_port);
	printf("udplogger debug: parameter minimum_target_age = '%lu'\n", conf.maximum_target_age);
	printf("udplogger debug: parameter prune_target_maximum_interval = '%ld'\n", conf.prune_target_maximum_interval);
#endif

	// Set up the socket that will be used to send logging data.
	fd = bind_socket(conf.listen_port);
	if (fd < 0)
	{
		fprintf(stderr, "could not setup logging socket\n");
		return -1;
	}
	
	// Initialize our send targets mutex.
	pthread_mutex_init(&targets_mutex, NULL);

	bzero(input_buffer, INPUT_BUFFER_SIZE * sizeof(char));
	pid = (LOGGER_PID_T)getpid();

	while (fgets((char *)input_buffer, INPUT_BUFFER_SIZE, stdin) != NULL)
	{
		/**
		 * We do not lock before accessing 'targets' on this initial check because we don't really care if it is an invalid check on occassion.
		 * Worst case scenarios are:
		 *  1) we think there are targets when there are not and we do a little extra work preparing the data for
		 *     sending when it is not going to be sent anywhere.
		 *  2) we think there are no targets when there are some, and we skip sending them a log entry or two (or even a few).
		 *
		 * This check is for efficiency only, and the two edge cases above won't have an appreciable impact on the accuracy
		 * of the logging system, but the check will save us a lot of unnecessary work and keep our idling impact on system
		 * resources low.
		 **/
		if (! targets)
		{
			continue;
		}

		input_line_length = trim(input_buffer);
		log_serial++;
	
		tmp = output_buffer;
	
		memcpy(tmp, &pid, sizeof(pid));
		tmp += sizeof(pid);
	
		memcpy(tmp, &log_serial, sizeof(log_serial));
		tmp += sizeof(log_serial);
	
		data_length = sizeof(output_buffer) - sizeof(pid) - sizeof(log_serial);
		result = compress2((Bytef *)tmp, (uLongf *)&data_length, input_buffer, input_line_length + 1, conf.compress_level);
		if (result == Z_OK)
		{
			pthread_mutex_lock(&targets_mutex);
			current = targets;
			while (current)
			{
				sendto(fd, output_buffer, LOG_PACKET_SIZE, 0, (struct sockaddr *) &(current->address), sizeof(current->address));
				current = current->next;
			}
			pthread_mutex_unlock(&targets_mutex);
		}
	}

	pthread_mutex_destroy(&targets_mutex);
	pthread_exit(NULL);
}


int arguments_parse(int argc, char **argv)
{
	int i;
	static struct option long_options[] =
	{
		{"compress", required_argument, 0, 'c'},
		{"help", no_argument, 0, 'h'},
		{"listen", required_argument, 0, 'l'},
		{"max_target_age", required_argument, 0, 'm'},
		{"prunt_target_maximum_interval", required_argument, 0, 'p'},
		{0, 0, 0, 0}
	};
	long long_tmp;
	int option_index;
	uintmax_t uint_tmp;

	// Initialize our configuration to the default settings.
	conf.listen_port = DEFAULT_LISTEN_PORT;
	conf.maximum_target_age = DEFAULT_MAXIMUM_TARGET_AGE;
	conf.prune_target_maximum_interval = DEFAULT_PRUNE_TARGET_MAXIMUM_INTERVAL;
	conf.compress_level = DEFAULT_COMPRESS_LEVEL;
	
	while (1)
	{	
		option_index = 0;
		i = getopt_long(argc, argv, "c:hl:m:p:", long_options, &option_index);
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
					conf.compress_level = uint_tmp;
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
					conf.listen_port = (uint16_t)uint_tmp;
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
				conf.maximum_target_age = uint_tmp;
				break;
			case 'p':
				long_tmp = strtol(optarg, 0, 10);
				if (! long_tmp || long_tmp == LONG_MIN || long_tmp == LONG_MAX)
				{
					fprintf(stderr, "invalid prune target maximum interval argument '%s'\n", optarg);
					return -1;
				}
				conf.prune_target_maximum_interval = long_tmp;
				break;
		}
	}

	return 1;
}


void arguments_show_usage()
{
	printf("Usage: udplogger [OPTIONS]\n");
	printf("\n");
	printf("  -c, --compress <level>                         gzip compression level to use (0-9, default %u)\n", DEFAULT_COMPRESS_LEVEL);
	printf("  -h, --help                                     display this help and exit\n");
	printf("  -l, --listen <port>                            listen for beacons on the given port (default %u)\n", DEFAULT_LISTEN_PORT);
	printf("  -m, --max_target_age <age>                     expire log targets after <age> seconds (default %lu)\n", DEFAULT_MAXIMUM_TARGET_AGE);
	printf("  -p, --prune_target_maximum_interval <interval> maximum interval in seconds between prunes of the log target list (default %ld)\n", DEFAULT_PRUNE_TARGET_MAXIMUM_INTERVAL);
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
