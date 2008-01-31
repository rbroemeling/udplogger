#include <getopt.h>
#include <inttypes.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "beacon.h"
#include "udplogger.h"
#include "trim.h"

// Parameter: the maximum age of a log target before it is expired.
uintmax_t maximum_target_age = DEFAULT_MAXIMUM_TARGET_AGE;

// Parameter: the port on which to listen for beacons.
uint16_t listen_port = DEFAULT_LISTEN_PORT;

// Global Variable: a singly-linked list of the log targets that we should report to.
struct log_target *targets = NULL;

int main (int argc, char **argv)
{
	int i;
	
	i = arguments_parse(argc, argv);
	if (i <= 0)
	{
		return i;
	}

	
	return 0;
}

int arguments_parse(int argc, char **argv)
{
	while (1)
	{
		static struct option long_options[] =
		{
			{"help", no_argument, 0, 'h'},
			{"listen", required_argument, 0, 'l'},
			{"max_target_age", required_argument, 0, 'm'},
			{0, 0, 0, 0}
		};
		int option_index = 0;
		uintmax_t uint_tmp;

		int i = getopt_long(argc, argv, "l:m:", long_options, &option_index);
		if (i == -1)
		{
			break;
		}
		switch (i)
		{
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
				else
				{
					if (uint_tmp > 0 && uint_tmp <= 0xFFFF)
					{
						listen_port = (uint16_t)uint_tmp;
					}
					else
					{
						fprintf(stderr, "listen argument %lu is out of port range (1-65535)\n", uint_tmp);
						return -1;
					}
				}
				break;
			case 'm':
				uint_tmp = strtoumax(optarg, 0, 10);
				if (! uint_tmp || uint_tmp == UINT_MAX)
				{
					fprintf(stderr, "invalid maximum target age argument '%s'\n", optarg);
					return -1;
				}
				else
				{
					maximum_target_age = uint_tmp;
				}
				break;
		}
	}
	
	return 1;
}

void arguments_show_usage()
{
	printf("Usage: udplogger [OPTIONS]\n");
	printf("\n");
	printf("  -h, --help                 display this help and exit\n");
	printf("  -l, --listen <port>        listen for beacons on the given port (default %u)\n", DEFAULT_LISTEN_PORT);
	printf("  -m, --max_target_age <age> expire log targets after <age> seconds (default %lu)\n", DEFAULT_MAXIMUM_TARGET_AGE);
	printf("\n");
}

// #define LOGGER_PID_T	u_int32_t
// #define LOGGER_CNT_T	u_int32_t

// #define LOGGER_BUFSIZE (1024)
// #define LOGGER_COMPRESSLEVEL 0

// this should not be modified (it includes the zlib buffer size)
// Packet is:
//	[u_int32_t pid][u_int32_t id][zlib data]
// #define LOGGER_ZBUFSZ (int)((LOGGER_BUFSIZE * 1.1) + 12)
// #define LOGGER_HDRSZ (sizeof(LOGGER_PID_T) + sizeof(LOGGER_CNT_T))
// #define LOGGER_SENDBUF (LOGGER_HDRSZ + LOGGER_ZBUFSZ)

//int main (int argc, char **argv)
//{
//	/**
//	 * should add command line parsing
//	 **/
//	struct sockaddr_in addr;
//	int fd, rv, size;
//	LOGGER_PID_T pid;
//	LOGGER_CNT_T cnt;
//	unsigned char linebuf[LOGGER_BUFSIZE];
//	unsigned char sendbuf[LOGGER_SENDBUF];
//	unsigned long buflen;
//	unsigned char *w;
//
//	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
//	{
//		perror("socket()");
//		return 255;
//	}
//
//	if (fcntl(fd, F_SETFL, O_NONBLOCK) == -1)
//	{
//		perror("fcntl(F_SETFL, O_NONBLOCK)");
//	}
//
//	memset(&addr, 0, sizeof(addr));
//	addr.sin_family = AF_INET;
//	addr.sin_addr.s_addr = inet_addr(LOGGER_GROUP);
//	addr.sin_port = htons(LOGGER_PORT);
//
//	memset(linebuf, 0, LOGGER_BUFSIZE);
//	memset(sendbuf, 0, LOGGER_SENDBUF);
//
//	pid = (LOGGER_PID_T)getpid();
//	cnt = 0;
//
//	while (fgets((char *)linebuf, LOGGER_BUFSIZE, stdin) != NULL)
//	{
//		buflen = trim(linebuf);
//		++cnt;
//		w = sendbuf;
//		memcpy(w, &pid, sizeof(pid)); w += sizeof(pid);
//		memcpy(w, &cnt, sizeof(cnt)); w += sizeof(cnt);
//		size = sizeof(sendbuf) - sizeof(pid) - sizeof(cnt);
//		rv = compress2((Bytef *)w, (uLongf *)&size, linebuf, buflen+1, LOGGER_COMPRESSLEVEL);
//		if (rv == Z_OK)
//		{
//			sendto(fd, sendbuf, (size + LOGGER_HDRSZ), 0, (struct sockaddr *) &addr, sizeof(addr));
//		}
//	}
//
//	return 0;
//}
