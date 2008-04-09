#ifndef __UDPLOGGER_H__
#define __UDPLOGGER_H__


/*
 * Structure that contains configuration information for the running instance
 * of udplogger.
 */
struct udplogger_configuration_t {
	uint16_t listen_port;
	uintmax_t maximum_target_age;
	long prune_target_interval;
	char *tag;
	uint8_t tag_length;
};


/*
 * Structure that is used to store information about the logging targets that
 * the running instance of udplogger is currently sending log data to.  One host
 * per structure, arranged as a singly-linked list.
 */
struct log_target_t {
	time_t beacon_timestamp;
	struct sockaddr_in address;
	struct log_target_t *next;
};

/* Global Variables (see udplogger.c). */
extern struct udplogger_configuration_t conf;
extern struct log_target_t *targets;
extern pthread_mutex_t targets_mutex;


#endif
