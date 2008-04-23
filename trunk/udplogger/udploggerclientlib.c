#include <arpa/inet.h>
#include <getopt.h>
#include <inttypes.h>
#include <net/if.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include "udplogger.h"
#include "socket.h"


/*
 * Please note that this file cannot link successfully by itself (or even with just socket.o).
 * This library implements a framework for sending beacons and receiving log information from
 * udploggerd logging hosts, but it is not complete in and of itself.  To implement a full client, this
 * library should be linked with another source file that gives an implementation of the function
 * log_packet_hook.
 *
 * Preferrably the log_packet_hook function should be kept simple enough to be inlined.
 *
 * See 'udploggercat.c' for a simple example.
 */


/*
 * Give a strndup prototype to stop compiler warnings.  GNU libraries give it to us,
 * but ISO C90 doesn't allow us to use _GNU_SOURCE before including <string.h>.
 */
char *strndup(const char *s, size_t n);


/*
 * Structure that is used to store information about the logging hosts that
 * the running instance of this udplogger client is currently sending beacons to.  One host
 * per structure, arranged as a singly-linked list.
 */
struct log_host_t {
	struct sockaddr_in address;
	struct log_host_t *next;
};


/*
 * Structure that contains configuration information for the running instance
 * of this udplogger client.
 */
struct udploggerclientlib_configuration_t {
	uintmax_t beacon_interval;
	struct log_host_t log_host;
} clientlib_conf;


/*
 * Default configuration values.  Used if not over-ridden by a command-line parameter.
 *
 * DEFAULT_BEACON_INTERVAL               The default interval at which beacons should be dispatched.
 */
#define DEFAULT_BEACON_INTERVAL          30UL


int add_log_host(struct sockaddr_in *);
int arguments_parse(int, char **);
void broadcast_scan();


/**
 * log_packet_hook(<source host>, <log line>)
 *
 * There is no implementation of this function in this file.  This function should be implemented in each
 * udploggerd client program, and should take the arguments sockaddr_in *sender (the source host of the log line)
 * and char *line (the log line data itself).  The hook function should not modify the data in either of those
 * arguments.
 **/
extern void inline log_packet_hook(const struct sockaddr_in *, const char *);


/**
 * main()
 *
 * Initializes the program configuration and state, then starts the beacon thread before
 * finally entering the main loop that reads log entries and dumps them to stdout.
 **/
int main (int argc, char **argv)
{
	fd_set all_set;
	char *beacon;
	char *buffer;
	struct log_host_t *log_host_ptr;
	int fd;
	fd_set read_set;
	int result = 0;
	struct sockaddr_in sender;
	socklen_t senderlen = sizeof(sender);
	struct timeval timeout;

	result = arguments_parse(argc, argv);
	if (result <= 0)
	{
		return result;
	}

#ifdef __DEBUG__
	printf("udploggerclientlib.c debug: parameter beacon_interval = '%lu'\n", clientlib_conf.beacon_interval);
	log_host_ptr = &clientlib_conf.log_host;
	while (log_host_ptr)
	{
		printf("udploggerclientlib.c debug: logging host %s:%u\n", inet_ntoa(log_host_ptr->address.sin_addr), ntohs(log_host_ptr->address.sin_port));
		log_host_ptr = log_host_ptr->next;
	}
#endif

	fd = bind_socket(0, 0);
	if (fd < 0)
	{
		fprintf(stderr, "udploggerclientlib.c could not setup socket\n");
		return -1;
	}

	buffer = calloc(PACKET_MAXIMUM_SIZE, sizeof(char));
	if (! buffer)
	{
		perror("udploggerclientlib.c calloc(buffer)");
		return -1;
	}

	beacon = calloc(BEACON_PACKET_SIZE, sizeof(char));
	if (! beacon)
	{
		perror("udploggerclientlib.c calloc(beacon)");
		return -1;
	}
	strncpy(beacon, BEACON_STRING, BEACON_PACKET_SIZE);
	beacon[BEACON_PACKET_SIZE - 1] = '\0';

	memset(&timeout, 0, sizeof(timeout));	

	FD_ZERO(&all_set);
	FD_SET(fd, &all_set);
	while (1)
	{
		read_set = all_set;
		result = select(fd+1, &read_set, NULL, NULL, &timeout);
		if (result < 0)
		{
			perror("udploggerclientlib.c select()");
			return -1;
		}
		else if (result == 0)
		{
			timeout.tv_sec = clientlib_conf.beacon_interval;
			timeout.tv_usec = 0L;
			log_host_ptr = &clientlib_conf.log_host;
			do
			{
				sendto(fd, beacon, BEACON_PACKET_SIZE, 0, (struct sockaddr *)&log_host_ptr->address, sizeof(log_host_ptr->address));
			} while ((log_host_ptr = log_host_ptr->next));
		}
		else if (result > 0)
		{
			if (FD_ISSET(fd, &read_set))
			{
				if (recvfrom(fd, buffer, PACKET_MAXIMUM_SIZE, 0, (struct sockaddr *)&sender, &senderlen) >= 0)
				{
					buffer[PACKET_MAXIMUM_SIZE - 1] = '\0';
					log_packet_hook(&sender, buffer);
				}
			}
		}
	}

	return 0;
}
	

/**
 * add_log_host(<log host sockaddr_in>)
 *
 * Simple utility function to take the host designated and add it to the list
 * of log hosts.  Returns 1 for success and 0 for failure.
 **/
int add_log_host(struct sockaddr_in *sin)
{
	struct log_host_t *log_host_ptr;
	
	log_host_ptr = &clientlib_conf.log_host;
	while (log_host_ptr->next)
	{
		log_host_ptr = log_host_ptr->next;
	}
	
	if (log_host_ptr->address.sin_family)
	{
		log_host_ptr->next = calloc(1, sizeof(struct log_host_t));
		if (! log_host_ptr->next)
		{
			perror("udploggerclientlib.c calloc(log_host)");
			return 0;
		}
		log_host_ptr = log_host_ptr->next;
	}
	
	log_host_ptr->address.sin_family = sin->sin_family;
	log_host_ptr->address.sin_addr.s_addr = sin->sin_addr.s_addr;
	log_host_ptr->address.sin_port = sin->sin_port;
	
#ifdef __DEBUG__
	printf("udploggerclientlib.c debug: added target %s:%hu\n", inet_ntoa(log_host_ptr->address.sin_addr), ntohs(log_host_ptr->address.sin_port));
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
	clientlib_conf.beacon_interval = DEFAULT_BEACON_INTERVAL;
	memset(&clientlib_conf.log_host, 0, sizeof(clientlib_conf.log_host));
	
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
				printf("Usage: %s [OPTIONS]\n", argv[0]);
				printf("\n");
				printf("  -h, --help                 display this help and exit\n");
				printf("  -o, --host <host>[:<port>] host and port to target with beacon transmissions (default broadcast)\n");
				printf("                             (default udplogger port is %u)\n", UDPLOGGER_DEFAULT_PORT);
				printf("  -i, --interval <interval>  interval in seconds between beacon transmissions (default %lu)\n", DEFAULT_BEACON_INTERVAL);
				printf("  -v, --version              display version and exit\n");          
				printf("\n");
				return 0;
			case 'i':
				uint_tmp = strtoumax(optarg, 0, 10);
				if (! uint_tmp || uint_tmp == UINT_MAX)
				{
					fprintf(stderr, "udploggerclientlib.c invalid beacon interval '%s'\n", optarg);
					return -1;
				}
				clientlib_conf.beacon_interval = uint_tmp;
				break;
			case 'o':
				char_ptr = strstr(optarg, ":");
				if (char_ptr == optarg)
				{
					fprintf(stderr, "udploggerclientlib.c invalid host specification '%s'\n", optarg);
					return -1;				
				}
#ifdef __DEBUG__
				printf("udploggerclientlib.c debug: parsing host target '%s'\n", optarg);
#endif
				
				if (char_ptr)
				{
					hostname_tmp = strndup(optarg, (char_ptr - optarg));
				}
				else
				{
					hostname_tmp = strdup(optarg);
				}
				if (! hostname_tmp)
				{
					fprintf(stderr, "udploggerclientlib.c could not allocate memory to record host '%s'\n", optarg);
					return -1;
				}
#ifdef __DEBUG__
				printf("udploggerclientlib.c debug:   determined hostname '%s'\n", hostname_tmp);
#endif

				if (char_ptr && *char_ptr == ':')
				{
					char_ptr++;
					uint_tmp = 0;
					if (char_ptr)
					{
						uint_tmp = strtoumax(char_ptr, 0, 10);
					}
					if (! uint_tmp || uint_tmp == UINT_MAX)
					{
						fprintf(stderr, "udploggerclientlib.c invalid port in host specification '%s'\n", optarg);
						free(hostname_tmp);
						return -1;
					}
				}
				else
				{
					uint_tmp = UDPLOGGER_DEFAULT_PORT;
				}
#ifdef __DEBUG__
				printf("udploggerclientlib.c debug:   determined port '%lu'\n", uint_tmp);
#endif

				hostent_ptr = gethostbyname(hostname_tmp);
				if (! hostent_ptr)
				{
					fprintf(stderr, "udploggerclientlib.c could not find the IP address of the host '%s'\n", hostname_tmp);
					free(hostname_tmp);
					return -1;
				}
				free(hostname_tmp);
				
				for (j = 0; hostent_ptr->h_addr_list[j] != NULL; j++)
				{
#ifdef __DEBUG__
					printf("udploggerclientlib.c debug:     considering address '%s', family '%hu'\n", inet_ntoa(*(struct in_addr *)(hostent_ptr->h_addr_list[j])), hostent_ptr->h_addrtype);
#endif
					if (hostent_ptr->h_addrtype == AF_INET)
					{
						sin.sin_family = hostent_ptr->h_addrtype;
						sin.sin_addr.s_addr = ((struct in_addr *)(hostent_ptr->h_addr_list[j]))->s_addr;
						sin.sin_port = htons(uint_tmp);
						
						add_log_host(&sin);
					}
				}
				break;
			case 'v':
				printf("udploggerclientlib.c revision r%d\n", REVISION);
				return 0;
		}
	}

	if (clientlib_conf.log_host.address.sin_family == 0)
	{
		/* No target hosts have been passed in.  Default is to add all broadcast addresses. */
		broadcast_scan();
	}

	if (clientlib_conf.log_host.address.sin_family == 0)
	{
		/* Final sanity check.  If we don't have any log hosts to target, then don't continue. */
		printf("udploggerclientlib.c no log targets\n");
		return -1;
	}
	
	return 1;
}


/**
 * broadcast_scan()
 *
 * Iterates through all interfaces on the system and adds all broadcast addresses found to the
 * clientlib_conf.log_host list.
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

	log_host_ptr = &clientlib_conf.log_host;

	ifc.ifc_len = max_interfaces * sizeof(struct ifreq);

	ifc.ifc_buf = calloc(1, ifc.ifc_len);
	if (! ifc.ifc_buf)
	{
		perror("udploggerclientlib.c calloc(ifc_buf)");
		return;
	}
	
	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0)
	{
		perror("udploggerclientlib.c socket()");
		return;
	}
	
	if (ioctl(fd, SIOCGIFCONF, &ifc) < 0)
	{
		perror("udploggerclientlib.c ioctl(SIOCGIFCONF)");
		if (close(fd))
		{
			perror("udploggerclientlib.c close()");
		}
		return;
	}
	
	ifr = ifc.ifc_req;
	num_interfaces = ifc.ifc_len / sizeof(struct ifreq);
	for (i = 0; i++ < num_interfaces; ifr++)
	{
#ifdef __DEBUG__
		printf("udploggerclientlib.c debug: found interface %s (%s)\n", ifr->ifr_name, inet_ntoa(((struct sockaddr_in *)&ifr->ifr_addr)->sin_addr));
#endif

		if (ifr->ifr_addr.sa_family != AF_INET)
		{
#ifdef __DEBUG__
			printf("udploggerclientlib.c debug:   %s is not of the family AF_INET\n", ifr->ifr_name);
#endif
			continue;
		}

		if (ioctl(fd, SIOCGIFFLAGS, ifr) < 0)
		{
			perror("udploggerclientlib.c ioctl(SIOCGIFFLAGS)");
			continue;
		}
		
		if (!(ifr->ifr_flags & IFF_UP))
		{
#ifdef __DEBUG__
			printf("udploggerclientlib.c debug:   %s flagged as down\n", ifr->ifr_name);
#endif
			continue;
		}
		if (ifr->ifr_flags & IFF_LOOPBACK)
		{
#ifdef __DEBUG__
			printf("udploggerclientlib.c debug:   %s is a loopback interface\n", ifr->ifr_name);
#endif
			continue;
		}
		if (ifr->ifr_flags & IFF_POINTOPOINT)
		{
#ifdef __DEBUG__
			printf("udploggerclientlib.c debug:   %s is a point-to-point interface\n", ifr->ifr_name);
#endif
			continue;
		}
		if (!(ifr->ifr_flags & IFF_BROADCAST))
		{
#ifdef __DEBUG__
			printf("udploggerclientlib.c debug:   %s does not have the broadcast flag set\n", ifr->ifr_name);
#endif
			continue;
		}
		
		if (ioctl(fd, SIOCGIFBRDADDR, ifr) < 0)
		{
			perror("udploggerclientlib.c ioctl(SIOCGIFBRDADDR)");
			continue;
		}
		
		memcpy(&sin, &(ifr->ifr_broadaddr), sizeof(ifr->ifr_broadaddr));
		if (sin.sin_addr.s_addr == INADDR_ANY)
		{
#ifdef __DEBUG__
			printf("udploggerclientlib.c debug:    %s is associated with INADDR_ANY\n", ifr->ifr_name);
#endif
			continue;
		}

		sin.sin_port = htons(UDPLOGGER_DEFAULT_PORT);
		add_log_host(&sin);
	}

	if (close(fd))
	{
		perror("udploggerclientlib.c close()");
	}
}
