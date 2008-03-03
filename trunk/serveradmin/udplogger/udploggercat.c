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
 * Structure that is used to store information about the logging targets that
 * the running instance of udploggercat is currently sending beacons to.  One host
 * per structure, arranged as a singly-linked list.
 */
struct log_target_t {
	struct sockaddr_in address;
	struct log_target_t *next;
};


/*
 * Structure that contains configuration information for the running instance
 * of udploggercat.
 */
struct udploggercat_configuration_t {
	uintmax_t beacon_interval;
	struct log_target_t target;
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
 * finally entering the main loop that reads stdin and outputs log entries to any hosts
 * on the target list.
 **/
int main (int argc, char **argv)
{
	struct log_target_t *debug_target_ptr = &conf.target;
	int result = 0;

	result = arguments_parse(argc, argv);
	if (result <= 0)
	{
		return result;
	}

#ifdef __DEBUG__
	printf("udploggercat.c debug: parameter beacon_interval = '%lu'\n", conf.beacon_interval);
	while (debug_target_ptr)
	{
		printf("udploggercat.c debug: logging target %s:%u\n", inet_ntoa(debug_target_ptr->address.sin_addr), debug_target_ptr->address.sin_port);
		debug_target_ptr = debug_target_ptr->next;
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
	struct hostent *host_tmp;
	char *hostname_tmp;
	int i, j;
	static struct option long_options[] =
	{
		{"help", no_argument, 0, 'h'},
		{"interval", required_argument, 0, 'i'},
		{"target", required_argument, 0, 't'},
		{"version", no_argument, 0, 'v'},
		{0, 0, 0, 0}
	};
	struct log_target_t *target_tmp;
	uintmax_t uint_tmp;
	
	/* Initialize our configuration to the default settings. */
	conf.beacon_interval = DEFAULT_BEACON_INTERVAL;
	memset(&conf.target, 0, sizeof(conf.target));
	target_tmp = &conf.target;
	
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
				printf("  -i, --interval <interval>  interval in seconds between beacon transmissions (default %lu)\n", DEFAULT_BEACON_INTERVAL);
				printf("  -t, --target <host>:<port> host and port to target with beacon transmissions (default broadcast)\n");
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
			case 't':
				char_ptr = strstr(optarg, ":");
				if (! char_ptr || (char_ptr == optarg))
				{
					fprintf(stderr, "udploggercat.c invalid target specification '%s'\n", optarg);
					return -1;				
				}
				
				hostname_tmp = strndup(optarg, (char_ptr - optarg));
				if (! hostname_tmp)
				{
					fprintf(stderr, "udploggercat.c could not allocate memory to record target host '%s'\n", optarg);
					return -1;
				}
				
				char_ptr++;
				if (char_ptr)
				{
					uint_tmp = strtoumax(char_ptr, 0, 10);
					if (! uint_tmp || uint_tmp == UINT_MAX)
					{
						fprintf(stderr, "udploggercat.c invalid port in target specification '%s'\n", optarg);
						free(hostname_tmp);
						return -1;
					}
				}
				else
				{
					fprintf(stderr, "udploggercat.c missing port in target specification '%s'\n", optarg);
					free(hostname_tmp);
					return -1;
				}		

				host_tmp = gethostbyname(hostname_tmp);
				if (! host_tmp)
				{
					fprintf(stderr, "udploggercat.c could not find the IP address of the target host '%s'\n", hostname_tmp);
					free(hostname_tmp);
					return -1;
				}
				free(hostname_tmp);
				
				for (j = 0; host_tmp->h_addr_list[j] != NULL; j++)
				{
					if (target_tmp->address.sin_family == AF_INET)
					{
						target_tmp->next = calloc(1, sizeof(struct log_target_t));
						if (! target_tmp->next)
						{
							perror("udploggercat.c calloc()");
							return -1;
						}
						target_tmp = target_tmp->next;
					}
					
					target_tmp->address.sin_family = AF_INET;
					target_tmp->address.sin_addr.s_addr = ((struct in_addr *)(host_tmp->h_addr_list[j]))->s_addr;
					target_tmp->address.sin_port = uint_tmp;
				}
				break;
			case 'v':
				printf("udploggercat.c revision r%d\n", REVISION);
				return 0;
		}
	}

	return 1;
}
