#include <arpa/inet.h>
#include <getopt.h>
#include <inttypes.h>
#include <net/if.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>
#include "udplogger.global.h"


int add_log_host(struct sockaddr_in *);
int arguments_parse(int, char **);
void *beacon_loop(void *);
void broadcast_scan();


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
	pthread_t beacon_thread;
	pthread_attr_t beacon_thread_attr;
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

	/* Start our beacon thread. */
	pthread_attr_init(&beacon_thread_attr);
	pthread_attr_setdetachstate(&beacon_thread_attr, PTHREAD_CREATE_DETACHED);
	result = pthread_create(&beacon_thread, &beacon_thread_attr, beacon_loop, NULL);
	pthread_attr_destroy(&beacon_thread_attr);
	if (result)
	{
		fprintf(stderr, "udploggercat.c could not start beacon thread.\n");
		return -1;
	}
	

	exit(0);
}


/**
 * add_log_host(sin)
 *
 * Simple utility function to take the host designated by sin and add it to the list
 * of log hosts.  Returns 1 for success and 0 for failure.
 **/
int add_log_host(struct sockaddr_in *sin)
{
	struct log_host_t *log_host_ptr;
	
	log_host_ptr = &conf.log_host;
	while (log_host_ptr->next)
	{
		log_host_ptr = log_host_ptr->next;
	}
	
	if (log_host_ptr->address.sin_family)
	{
		log_host_ptr->next = calloc(1, sizeof(struct log_host_t));
		if (! log_host_ptr->next)
		{
			perror("udploggercat.c calloc()");
			return 0;
		}
		log_host_ptr = log_host_ptr->next;
	}
	
	log_host_ptr->address.sin_family = sin->sin_family;
	log_host_ptr->address.sin_addr.s_addr = sin->sin_addr.s_addr;
	log_host_ptr->address.sin_port = sin->sin_port;
	
#ifdef __DEBUG__
	printf("udploggercat.c debug: added target %s:%hu\n", inet_ntoa(log_host_ptr->address.sin_addr), log_host_ptr->address.sin_port);
#endif

	return 1;
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
	struct sockaddr_in sin;
	uintmax_t uint_tmp;
	
	/* Initialize our configuration to the default settings. */
	conf.beacon_interval = DEFAULT_BEACON_INTERVAL;
	memset(&conf.log_host, 0, sizeof(conf.log_host));
	
	while (1)
	{	
		i = getopt_long(argc, argv, "hi:o:v", long_options, NULL);
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
#ifdef __DEBUG__
				printf("udploggercat.c debug: parsing host target '%s'\n", optarg);
#endif
				
				hostname_tmp = strndup(optarg, (char_ptr - optarg));
				if (! hostname_tmp)
				{
					fprintf(stderr, "udploggercat.c could not allocate memory to record host '%s'\n", optarg);
					return -1;
				}
#ifdef __DEBUG__
				printf("udploggercat.c debug:   determined hostname '%s'\n", hostname_tmp);
#endif

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
#ifdef __DEBUG__
				printf("udploggercat.c debug:   determined port '%lu'\n", uint_tmp);
#endif

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
#ifdef __DEBUG__
					printf("udploggercat.c debug:     considering address '%s', family '%hu'\n", inet_ntoa(*(struct in_addr *)(hostent_ptr->h_addr_list[j])), hostent_ptr->h_addrtype);
#endif
					if (hostent_ptr->h_addrtype == AF_INET)
					{
						sin.sin_family = hostent_ptr->h_addrtype;
						sin.sin_addr.s_addr = ((struct in_addr *)(hostent_ptr->h_addr_list[j]))->s_addr;
						sin.sin_port = uint_tmp;
						
						add_log_host(&sin);
					}
				}
				break;
			case 'v':
				printf("udploggercat.c revision r%d\n", REVISION);
				return 0;
		}
	}

	if (conf.log_host.address.sin_family == 0)
	{
		/* No target hosts have been passed in.  Default is to add all broadcast addresses. */
		broadcast_scan();
	}

	return 1;
}


/**
 * beacon_loop()
 *
 * The main thread loop for the beacon thread.  Creates a socket and sends beacon packets over it every
 * conf.beacon_interval.
 **/
void *beacon_loop(void *arg)
{

}


/**
 * broadcast_Scan()
 *
 * Iterates through all interfaces on the system and adds all broadcast addresses found to the
 * conf.log_host list.
 **/
void broadcast_scan()
{
	int fd;
	int i;
	struct ifconf ifc;
	struct ifreq *ifr;
	struct log_host_t *log_host_ptr;
	int max_interfaces = 32; /* The maximum number of interfaces that we should obtain configuration for. */
	int num_interfaces = 0; /* The number of interfaces that we have actually found. */
	struct sockaddr_in sin;

	log_host_ptr = &conf.log_host;

	ifc.ifc_len = max_interfaces * sizeof(struct ifreq);

	ifc.ifc_buf = calloc(1, ifc.ifc_len);
	if (! ifc.ifc_buf)
	{
		perror("udploggercat.c calloc(ifc_buf)");
		return;
	}
	
	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0)
	{
		perror("udploggercat.c socket()");
		return;
	}
	
	if (ioctl(fd, SIOCGIFCONF, &ifc) < 0)
	{
		perror("udploggercat.c ioctl(SIOCGIFCONF)");
		if (close(fd))
		{
			perror("udploggercat.c close()");
		}
		return;
	}
	
	ifr = ifc.ifc_req;
	num_interfaces = ifc.ifc_len / sizeof(struct ifreq);
	for (i = 0; i++ < num_interfaces; ifr++)
	{
#ifdef __DEBUG__
		printf("udploggercat.c debug: found interface %s (%s)\n", ifr->ifr_name, inet_ntoa(((struct sockaddr_in *)&ifr->ifr_addr)->sin_addr));
#endif
		if (ioctl(fd, SIOCGIFFLAGS, ifr) < 0)
		{
			perror("udploggercat.c ioctl(SIOCGIFFLAGS)");
			continue;
		}
		
		if (!(ifr->ifr_flags & IFF_UP))
		{
#ifdef __DEBUG__
			printf("udploggercat.c debug:   %s flagged as down\n", ifr->ifr_name);
#endif
			continue;
		}
		if (ifr->ifr_flags & IFF_LOOPBACK)
		{
#ifdef __DEBUG__
			printf("udploggercat.c debug:   %s is a loopback interface\n", ifr->ifr_name);
#endif
			continue;
		}
		if (ifr->ifr_flags & IFF_POINTOPOINT)
		{
#ifdef __DEBUG__
			printf("udploggercat.c debug:   %s is a point-to-point interface\n", ifr->ifr_name);
#endif
			continue;
		}
		if (!(ifr->ifr_flags & IFF_BROADCAST))
		{
#ifdef __DEBUG__
			printf("udploggercat.c debug:   %s does not have the broadcast flag set\n", ifr->ifr_name);
#endif
			continue;
		}
		
		if (ioctl(fd, SIOCGIFBRDADDR, ifr) < 0)
		{
			perror("udploggercat.c ioctl(SIOCGIFBRDADDR)");
			continue;
		}
		
		memcpy(&sin, &(ifr->ifr_broadaddr), sizeof(ifr->ifr_broadaddr));
		if (sin.sin_addr.s_addr == INADDR_ANY)
		{
#ifdef __DEBUG__
			printf("udploggercat.c debug:    %s is associated with INADDR_ANY\n", ifr->ifr_name);
#endif
			continue;
		}
		
		sin.sin_port = UDPLOGGER_DEFAULT_PORT;
		add_log_host(&sin);
	}

	if (close(fd))
	{
		perror("udploggercat.c close()");
	}
}
