#include <getopt.h>
#include <inttypes.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>
#include "beacon.h"
#include "socket.h"
#include "trim.h"
#include "udplogger.h"


int arguments_parse(int, char **);


/*
 * Global Variable Declarations
 *
 * conf          is used to store the configuration of the currently-running udplogger process.
 * targets       is used to store the list of destinations that this process is currently sending data to.
 * targets_mutex is a mutex used to control access to the targets variable across threads.
 */
struct udplogger_configuration_t conf;
struct log_target_t *targets = NULL;
pthread_mutex_t targets_mutex;


/*
 * Default configuration values.  Used if not over-ridden by a command-line parameter.
 *
 * DEFAULT_COMPRESS_LEVEL                The default level of compression to use for outgoing logging data.
 * DEFAULT_LISTEN_PORT                   The default port to both listen on (for beacons) and send (logging data) from.
 * DEFAULT_MAXIMUM_TARGET_AGE            The default maximum age of a destination on the target list before it is removed.
 * DEFAULT_PRUNE_TARGET_MAXIMUM_INTERVAL The default (maximum) interval between prunes of the target list.
 */
#define DEFAULT_COMPRESS_LEVEL                0
#define DEFAULT_LISTEN_PORT                   43824U
#define DEFAULT_MAXIMUM_TARGET_AGE            120UL
#define DEFAULT_PRUNE_TARGET_MAXIMUM_INTERVAL 10L

/**
 * main()
 *
 * Initializes the program configuration and state, then starts the beacon thread before
 * finally entering the main loop that reads stdin and outputs log entries to any hosts
 * on the target list.
 **/
int main (int argc, char **argv)
{
	pthread_t beacon_thread;
	pthread_attr_t beacon_thread_attr;
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
	printf("udplogger.c debug: parameter compress_level = '%u'\n", conf.compress_level);
	printf("udplogger.c debug: parameter listen_port = '%u'\n", conf.listen_port);
	printf("udplogger.c debug: parameter minimum_target_age = '%lu'\n", conf.maximum_target_age);
	printf("udplogger.c debug: parameter prune_target_maximum_interval = '%ld'\n", conf.prune_target_maximum_interval);
#endif

	/* Set up the socket that will be used to send logging data. */
	fd = bind_socket(conf.listen_port);
	if (fd < 0)
	{
		fprintf(stderr, "udplogger.c could not setup logging socket\n");
		return -1;
	}
	
	/* Initialize our send targets mutex. */
	pthread_mutex_init(&targets_mutex, NULL);

	/* Start our beacon-listener thread. */
	pthread_attr_init(&beacon_thread_attr);
	pthread_attr_setdetachstate(&beacon_thread_attr, PTHREAD_CREATE_DETACHED);
	result = pthread_create(&beacon_thread, &beacon_thread_attr, beacon_main, NULL);
	pthread_attr_destroy(&beacon_thread_attr);
	if (result)
	{
		fprintf(stderr, "udplogger.c could not start beacon thread.\n");
		return -1;
	}
	
	memset(input_buffer, 0, INPUT_BUFFER_SIZE * sizeof(char));
	pid = (LOGGER_PID_T)getpid();

	while (fgets((char *)input_buffer, INPUT_BUFFER_SIZE, stdin) != NULL)
	{
		/*
		 * We do not lock before accessing 'targets' on this initial check because we don't really care if it is an invalid check on occassion.
		 * Worst case scenarios are:
		 *  1) we think there are targets when there are not and we do a little extra work preparing the data for
		 *     sending when it is not going to be sent anywhere.
		 *  2) we think there are no targets when there are some, and we skip sending them a log entry or two (or even a few).
		 *
		 * This check is for efficiency only, and the two edge cases above won't have an appreciable impact on the accuracy
		 * of the logging system, but the check will save us a lot of unnecessary work and keep our idling impact on system
		 * resources low.
		 */
		if (! targets)
		{
			continue;
		}

		input_line_length = trim(input_buffer, INPUT_BUFFER_SIZE - 1);
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

	/* Don't bother cleaning up our mutex, threads, etc.  The exit procedures will take care of it. */
	exit(0);
}


/**
 * arguments_parse(argc, argv)
 *
 * Utility function to parse the passed in arguments using getopt; also responsible for sanity-checking
 * any passed-in values and ensuring that the program's configuration is sane before returning (i.e.
 * using default values).
 **/
int arguments_parse(int argc, char **argv)
{
	int i;
	static struct option long_options[] =
	{
		{"compress", required_argument, 0, 'c'},
		{"help", no_argument, 0, 'h'},
		{"listen", required_argument, 0, 'l'},
		{"max_target_age", required_argument, 0, 'm'},
		{"prune_target_maximum_interval", required_argument, 0, 'p'},
		{0, 0, 0, 0}
	};
	long long_tmp;
	int option_index;
	uintmax_t uint_tmp;

	/* Initialize our configuration to the default settings. */
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
					fprintf(stderr, "udplogger.c invalid compress level argument '%s'\n", optarg);
					return -1;
				}
				if (uint_tmp >= 0 && uint_tmp <= 9)
				{
					conf.compress_level = uint_tmp;
				}
				else
				{
					fprintf(stderr, "udplogger.c compression level %lu is out of range (0-9)\n", uint_tmp);
				}
				break;
			case 'h':
				printf("Usage: udplogger [OPTIONS]\n");
				printf("\n");
				printf("  -c, --compress <level>                         gzip compression level to use (0-9, default %d)\n", DEFAULT_COMPRESS_LEVEL);
				printf("  -h, --help                                     display this help and exit\n");
				printf("  -l, --listen <port>                            listen for beacons on the given port (default %u)\n", DEFAULT_LISTEN_PORT);
				printf("  -m, --max_target_age <age>                     expire log targets after <age> seconds (default %lu)\n", DEFAULT_MAXIMUM_TARGET_AGE);
				printf("  -p, --prune_target_maximum_interval <interval> maximum interval in seconds between prunes of the log target list (default %ld)\n", DEFAULT_PRUNE_TARGET_MAXIMUM_INTERVAL);
				printf("\n");
				return 0;
			case 'l':
				uint_tmp = strtoumax(optarg, 0, 10);
				if (! uint_tmp || uint_tmp == UINT_MAX)
				{
					fprintf(stderr, "udplogger.c invalid listen port argument '%s'\n", optarg);
					return -1;
				}
				if (uint_tmp > 0 && uint_tmp <= 0xFFFF)
				{
					conf.listen_port = (uint16_t)uint_tmp;
				}
				else
				{
					fprintf(stderr, "udplogger.c listen argument %lu is out of port range (1-65535)\n", uint_tmp);
					return -1;
				}
				break;
			case 'm':
				uint_tmp = strtoumax(optarg, 0, 10);
				if (! uint_tmp || uint_tmp == UINT_MAX)
				{
					fprintf(stderr, "udplogger.c invalid maximum target age argument '%s'\n", optarg);
					return -1;
				}
				conf.maximum_target_age = uint_tmp;
				break;
			case 'p':
				long_tmp = strtol(optarg, 0, 10);
				if (! long_tmp || long_tmp == LONG_MIN || long_tmp == LONG_MAX)
				{
					fprintf(stderr, "udplogger.c invalid prune target maximum interval argument '%s'\n", optarg);
					return -1;
				}
				conf.prune_target_maximum_interval = long_tmp;
				break;
		}
	}

	return 1;
}
