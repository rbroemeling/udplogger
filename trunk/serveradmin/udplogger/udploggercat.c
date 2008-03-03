#include <arpa/inet.h>
#include <getopt.h>
#include <inttypes.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>


int arguments_parse(int, char **);


/*
 * Give a strndup prototype to stop compiler warnings.  GNU libraries give it to us,
 * but ISO C90 doesn't allow us to use _GNU_SOURCE before including <string.h>.
 */
char *strndup(const char *s, size_t n);


/*
 * Structure that is used to store information about the logging hosts that
 * the running instance of udploggercat is currently sending beacons to.  One host
 * per structure, arranged as a singly-linked list.
 */
struct log_host_t {
	struct sockaddr_in address;
	struct log_host_t *next;
};


/*
 * Structure that contains configuration information for the running instance
 * of udploggercat.
 */
struct udploggercat_configuration_t {
	uintmax_t beacon_interval;
	struct log_host_t log_host;
} conf;


/*
 * Default configuration values.  Used if not over-ridden by a command-line parameter.
 *
 * DEFAULT_BEACON_INTERVAL               The default interval at which beacons should be dispatched.
 */
#define DEFAULT_BEACON_INTERVAL               30UL


/**
 * main()
 *
 * Initializes the program configuration and state, then starts the beacon thread before
 * finally entering the main loop that reads log entries and dumps them to stdout.
 **/
int main (int argc, char **argv)
{
	struct log_host_t *debug_log_host_ptr = &conf.log_host;
	int result = 0;

	result = arguments_parse(argc, argv);
	if (result <= 0)
	{
		return result;
	}

#ifdef __DEBUG__
	printf("udploggercat.c debug: parameter beacon_interval = '%lu'\n", conf.beacon_interval);
	while (debug_log_host_ptr)
	{
		printf("udploggercat.c debug: logging host %s:%u\n", inet_ntoa(debug_log_host_ptr->address.sin_addr), debug_log_host_ptr->address.sin_port);
		debug_log_host_ptr = debug_log_host_ptr->next;
	}
#endif

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
	char *char_ptr;
	struct hostent *hostent_ptr;
	char *hostname_tmp;
	int i, j;
	static struct option long_options[] =
	{
		{"help", no_argument, 0, 'h'},
		{"host", required_argument, 0, 'o'},
		{"interval", required_argument, 0, 'i'},
		{"version", no_argument, 0, 'v'},
		{0, 0, 0, 0}
	};
	struct log_host_t *log_host_ptr;
	uintmax_t uint_tmp;
	
	/* Initialize our configuration to the default settings. */
	conf.beacon_interval = DEFAULT_BEACON_INTERVAL;
	memset(&conf.log_host, 0, sizeof(conf.log_host));
	log_host_ptr = &conf.log_host;
	
	while (1)
	{	
		i = getopt_long(argc, argv, "hi:t:v", long_options, NULL);
		if (i == -1)
		{
			break;
		}
		switch (i)
		{
			case 'h':
				printf("Usage: udploggercat [OPTIONS]\n");
				printf("\n");
				printf("  -h, --help                 display this help and exit\n");
				printf("  -o, --host <host>:<port>   host and port to target with beacon transmissions (default broadcast)\n");
				printf("  -i, --interval <interval>  interval in seconds between beacon transmissions (default %lu)\n", DEFAULT_BEACON_INTERVAL);
				printf("  -v, --version              display udploggercat version and exit\n");          
				printf("\n");
				return 0;
			case 'i':
				uint_tmp = strtoumax(optarg, 0, 10);
				if (! uint_tmp || uint_tmp == UINT_MAX)
				{
					fprintf(stderr, "udploggercat.c invalid beacon interval '%s'\n", optarg);
					return -1;
				}
				conf.beacon_interval = uint_tmp;
				break;
			case 'o':
				char_ptr = strstr(optarg, ":");
				if (! char_ptr || (char_ptr == optarg))
				{
					fprintf(stderr, "udploggercat.c invalid host specification '%s'\n", optarg);
					return -1;				
				}
				
				hostname_tmp = strndup(optarg, (char_ptr - optarg));
				if (! hostname_tmp)
				{
					fprintf(stderr, "udploggercat.c could not allocate memory to record host '%s'\n", optarg);
					return -1;
				}
				
				char_ptr++;
				if (char_ptr)
				{
					uint_tmp = strtoumax(char_ptr, 0, 10);
					if (! uint_tmp || uint_tmp == UINT_MAX)
					{
						fprintf(stderr, "udploggercat.c invalid port in host specification '%s'\n", optarg);
						free(hostname_tmp);
						return -1;
					}
				}
				else
				{
					fprintf(stderr, "udploggercat.c missing port in host specification '%s'\n", optarg);
					free(hostname_tmp);
					return -1;
				}		

				hostent_ptr = gethostbyname(hostname_tmp);
				if (! hostent_ptr)
				{
					fprintf(stderr, "udploggercat.c could not find the IP address of the host '%s'\n", hostname_tmp);
					free(hostname_tmp);
					return -1;
				}
				free(hostname_tmp);
				
				for (j = 0; hostent_ptr->h_addr_list[j] != NULL; j++)
				{
					if (log_host_ptr->address.sin_family == AF_INET)
					{
						log_host_ptr->next = calloc(1, sizeof(struct log_host_t));
						if (! log_host_ptr->next)
						{
							perror("udploggercat.c calloc()");
							return -1;
						}
						log_host_ptr = log_host_ptr->next;
					}
					
					log_host_ptr->address.sin_family = AF_INET;
					log_host_ptr->address.sin_addr.s_addr = ((struct in_addr *)(hostent_ptr->h_addr_list[j]))->s_addr;
					log_host_ptr->address.sin_port = uint_tmp;
				}
				break;
			case 'v':
				printf("udploggercat.c revision r%d\n", REVISION);
				return 0;
		}
	}

	return 1;
}
