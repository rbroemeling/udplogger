#include <arpa/inet.h>
#include <errno.h>
#include <getopt.h>
#include <inttypes.h>
#include <net/if.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include "udplogger.h"
#include "udploggerclientlib.h"
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
 * See 'udploggerc.c' for a simple example.
 */


/*
 * Give a strndup prototype to stop compiler warnings.  GNU libraries give it to us,
 * but ISO C90 doesn't allow us to use _GNU_SOURCE before including <string.h>.
 */
char *strndup(const char *, size_t);


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
 * Structure that holds a collection of signal flags.  A signal will be a member
 * of this set if the signal has been received/has not been dealt with.  This
 * set should be checked for signals to be handled and if they are present,
 * the condition should be handled and the signal should be removed from the set.
 */
static sigset_t signal_flags;


/*
 * Structure that contains configuration information for the running instance
 * of this udplogger library.
 */
struct udploggerclientlib_configuration_t {
	uintmax_t beacon_interval;
	struct log_host_t log_host;
} udploggerclientlib_conf;


/*
 * Default configuration values.  Used if not over-ridden by a command-line parameter.
 *
 * DEFAULT_BEACON_INTERVAL               The default interval at which beacons should be dispatched.
 */
#define DEFAULT_BEACON_INTERVAL          30UL


int add_log_host(struct sockaddr_in *);
int add_option(const char *, const int, const char);
int arguments_parse(int, char **);
void broadcast_scan();
static void sig_handler(int);


static struct option *long_options = NULL;
char *short_options = NULL;


/**
 * main()
 *
 * Initializes the program configuration and state, then enters a main loop that reads log entries
 * (calling the log_packet_hook function on each) and sends beacon packets.
 **/
int main (int argc, char **argv)
{
	fd_set all_set;
	sigset_t blocked_signals;
	char beacon[BEACON_PACKET_SIZE];
	char buffer[PACKET_MAXIMUM_SIZE];
	int fd;
	struct log_host_t *log_host_ptr;
	sigset_t original_signal_mask;
	fd_set read_set;
	int result = 0;
	struct sockaddr_in sender;
	socklen_t senderlen = sizeof(sender);

	/*
	 * Create a signal set that consists of all of the signals except for SIGKILL,
	 * SIGSEGV, and SIGSTOP.  Do not include those signals because blocking them
	 * doesn't work (for SIGKILL and SIGSTOP) or they are too important to block
	 * (for SIGSEGV).
	 */
	if (sigfillset(&blocked_signals))
	{
		perror("udploggerclientlib.c sigfillset()");
		return -1;
	}
	if (sigdelset(&blocked_signals, SIGKILL) || sigdelset(&blocked_signals, SIGSEGV) || sigdelset(&blocked_signals, SIGSTOP))
	{
		perror("udploggerclientlib.c sigdelset()");
		return -1;
	}

	/*
	 * Block all of the signals in blocked_signals from being delivered to this
	 * process.  Save the current/original signal mask of this process
	 * to original_signal_mask.
	 */
	if (sigemptyset(&original_signal_mask))
	{
		perror("udploggerclientlib.c sigemptyset()");
		return -1;
	}
	if (sigprocmask(SIG_BLOCK, &blocked_signals, &original_signal_mask))
	{
		perror("udploggerclientlib.c sigprocmask()");
		return -1;
	}

	/*
	 * Clear out the list of signals that have been received by this process,
	 * so that every signal reads as "not received".
	 */
	if (sigemptyset(&signal_flags))
	{
		perror("udploggerclientlib.c sigemptyset()");
		return -1;
	}

	/*
	 * Install signal handlers that will record the receipt of signals that
	 * we are interested in.
	 */
	if (signal(SIGALRM, sig_handler) == SIG_ERR)
	{
		perror("udploggerclientlib.c signal(SIGALRM)");
		return -1;
	}
	if (signal(SIGTERM, sig_handler) == SIG_ERR)
	{
		perror("udploggerclientlib.c signal(SIGTERM)");
		return -1;
	}
	if (signal(SIGHUP, sig_handler) == SIG_ERR)
	{
		perror("udploggerclientlib.c signal(SIGHUP)");
		return -1;
	}

	result = arguments_parse(argc, argv);
	if (result <= 0)
	{
		return result;
	}

#ifdef __DEBUG__
	printf("udploggerclientlib.c debug: parameter beacon_interval = '%lu'\n", udploggerclientlib_conf.beacon_interval);
	log_host_ptr = &udploggerclientlib_conf.log_host;
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

	strncpy(beacon, BEACON_STRING, BEACON_PACKET_SIZE);
	beacon[BEACON_PACKET_SIZE - 1] = '\0';

	/*
	 * Send an ALARM signal to ourselves so that we immediately broadcast a beacon
	 * on startup.  Note that this signal is currently blocked but will be delivered
	 * as soon as we unblock signals (i.e. on pselect).
	 */
	raise(SIGALRM);

	FD_ZERO(&all_set);
	FD_SET(fd, &all_set);
	while (1)
	{
		read_set = all_set;
		/*
		 * Wait for either a log packet to arrive or the receipt of a
		 * signal.
		 */
		result = pselect(fd+1, &read_set, NULL, NULL, NULL, &original_signal_mask);

		if (result < 0)
		{
			/*
			 * If we were not interrupted by a signal, die with a pselect() error.
			 */
			if (errno != EINTR)
			{
				perror("udploggerclientlib.c pselect()");
				return -1;
			}

			/*
			 * If we have received a SIGALRM, send out a beacon and then start another
			 * alarm counter so that it will trigger to send out the next one.
			 */
			if (sigismember(&signal_flags, SIGALRM))
			{
				#ifdef __DEBUG__
					printf("udploggerclientlib.c debug: ALRM received, sending beacon\n");
				#endif
				log_host_ptr = &udploggerclientlib_conf.log_host;
				do
				{
					sendto(fd, beacon, BEACON_PACKET_SIZE, 0, (struct sockaddr *)&log_host_ptr->address, sizeof(log_host_ptr->address));
				} while ((log_host_ptr = log_host_ptr->next));
				alarm(udploggerclientlib_conf.beacon_interval);
			}

			handle_signal_hook(&signal_flags);
		}
		else if (FD_ISSET(fd, &read_set))
		{
			if (recvfrom(fd, buffer, PACKET_MAXIMUM_SIZE, 0, (struct sockaddr *)&sender, &senderlen) >= 0)
			{
				buffer[PACKET_MAXIMUM_SIZE - 1] = '\0';
				log_packet_hook(&sender, buffer);
			}
		}

		if (sigismember(&signal_flags, SIGTERM))
		{
			#ifdef __DEBUG__
				printf("udploggerclientlib.c debug: exiting normally\n");
			#endif
			break;
		}

		/*
		 * Clear out the list of signals that have been received by this process,
		 * so that every signal reads as "not received".
		 */
		if (sigemptyset(&signal_flags))
		{
			perror("udploggerclientlib.c sigemptyset()");
			return -1;
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

	log_host_ptr = &udploggerclientlib_conf.log_host;
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
 * add_option(long_option, has_arg, short_option)
 *
 * Utility function to add the given option to the list of possibilities for getopt.  Returns
 * 1 on success or 0 on failure (upon malloc failure).  Note that this function does not completely
 * clean up after itself on a failure... it is assumed that the program will not continue if an
 * add_option call fails.
 **/
int add_option(const char *long_option, const int has_arg, const char short_option)
{
	int i, j;
	static unsigned int num_options = 0;
	char *tmp;

#ifdef __DEBUG__
	if (long_option == NULL && has_arg == 0 && short_option == 0)
	{
		printf("udploggerclientlib.c debug: marking end of the option array (after %d options)\n", num_options);
	}
	else
	{
		printf("udploggerclientlib.c debug: adding option --%s/-%c (has_arg %d)\n", long_option, short_option, has_arg);
	}
#endif

	if (short_options == NULL)
	{
		short_options = calloc(1, sizeof(char));
		if (! short_options)
		{
			perror("udploggerclientlib.c calloc(short_options)");
			return 0;
		}
	}

	i = num_options;
	num_options++;
	long_options = realloc(long_options, num_options * sizeof(struct option));
	if (! long_options)
	{
		perror("udploggerclientlib.c realloc(long_options)");
		return 0;
	}

	if (long_option != NULL || has_arg != 0 || short_option != 0)
	{
		j = strlen(short_options);
		if (has_arg == required_argument)
		{
			tmp = realloc(short_options, (j + 1 + 2) * sizeof(char));
			if (tmp)
			{
				tmp[j] = short_option;
				tmp[j+1] = ':';
				tmp[j+2] = '\0';
			}
		}
		else if (has_arg == optional_argument)
		{
			tmp = realloc(short_options, (j + 1 + 3) * sizeof(char));
			if (tmp)
			{
				short_options[j] = short_option;
				short_options[j+1] = ':';
				short_options[j+2] = ':';
				short_options[j+3] = '\0';
			}
		}
		else if (has_arg == no_argument)
		{
			tmp = realloc(short_options, (j + 1 + 1) * sizeof(char));
			if (tmp)
			{
				short_options[j] = short_option;
				short_options[j+1] = '\0';
			}
		}
		if (tmp)
		{
			short_options = tmp;
		}
		else
		{
			perror("udploggerclientlib.c realloc(short_options)");
			return 0;
		}
	}

	long_options[i].name = NULL;
	if (long_option)
	{
		long_options[i].name = strdup(long_option);
		if (! long_options[i].name)
		{
			perror("udploggerclientlib.c strdup(long_option)");
			return 0;
		}
	}
	long_options[i].has_arg = has_arg;
	long_options[i].flag = 0;
	long_options[i].val = short_option;

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

	struct sockaddr_in sin;
	uintmax_t uint_tmp;

	/* Initialize our configuration to the default settings. */
	udploggerclientlib_conf.beacon_interval = DEFAULT_BEACON_INTERVAL;
	memset(&udploggerclientlib_conf.log_host, 0, sizeof(udploggerclientlib_conf.log_host));

	if (! add_option("help", no_argument, 'h'))
	{
		return -1;
	}
	if (! add_option("host", required_argument, 'o'))
	{
		return -1;
	}
	if (! add_option("interval", required_argument, 'i'))
	{
		return -1;
	}
	if (! add_option("version", no_argument, 'v'))
	{
		return -1;
	}
	if (! add_option_hook())
	{
		return -1;
	}
	if (! add_option(NULL, 0, 0))
	{
		return -1;
	}

	while (1)
	{
		i = getopt_long(argc, argv, short_options, long_options, NULL);
		if (i == -1)
		{
			break;
		}
		switch (i)
		{
			case 'h':
				printf("Usage: %s [OPTIONS]\n", argv[0]);
				printf("\n");
				printf("General Library Options\n");
				printf("  -h, --help                        display this help and exit\n");
				printf("  -o, --host <host>[:<port>]        host and port to target with beacon transmissions (default broadcast)\n");
				printf("                                    (default udplogger port is %u)\n", UDPLOGGER_DEFAULT_PORT);
				printf("  -i, --interval <interval>         interval in seconds between beacon transmissions (default %lu)\n", DEFAULT_BEACON_INTERVAL);
				printf("  -v, --version                     display version and exit\n");
				printf("\n");
				printf("%s Specific Options\n", argv[0]);
				usage_hook();
				return 0;
			case 'i':
				uint_tmp = strtoumax(optarg, 0, 10);
				if (! uint_tmp || uint_tmp == UINT_MAX)
				{
					fprintf(stderr, "udploggerclientlib.c invalid beacon interval '%s'\n", optarg);
					return -1;
				}
				udploggerclientlib_conf.beacon_interval = uint_tmp;
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
			case '?':
				break;
			default:
				j = getopt_hook(i);
				if (! j)
				{
					fprintf(stderr, "udploggerclientlib.c unhandled option '%c'\n", i);
					return -1;
				}
				else if (j < 0)
				{
					return j;
				}
				break;
		}
	}

	if (udploggerclientlib_conf.log_host.address.sin_family == 0)
	{
		/* No target hosts have been passed in.  Default is to add all broadcast addresses. */
		broadcast_scan();
	}

	if (udploggerclientlib_conf.log_host.address.sin_family == 0)
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
 * udploggerclientlib_conf.log_host list.
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

	log_host_ptr = &udploggerclientlib_conf.log_host;

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


/**
 * sig_handler(int signal_number)
 *
 * Generic signal handler -- just adds the received signal to our set of
 * signals received and then returns.
 */
static void sig_handler(int signal_number)
{
	#ifdef __DEBUG__
		printf("udploggerclientlib.c debug: received signal %d\n", signal_number);
	#endif
	if (sigaddset(&signal_flags, signal_number))
	{
		perror("udploggerclientlib.c sigaddset()");
	}
}
