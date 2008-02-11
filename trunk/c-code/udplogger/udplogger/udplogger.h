#ifndef __UDPLOGGER_H__
#define __UDPLOGGER_H__


/*
 * Structure that contains configuration information for the running instance
 * of udplogger.
 */
struct udplogger_configuration_t {
	uint8_t compress_level;
	uint16_t listen_port;
	uintmax_t maximum_target_age;
	long prune_target_maximum_interval;
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


/* The maximum length of a single log line (as read from stdin). */
#define INPUT_BUFFER_SIZE 1024


/* The type used to contain the PID of the udplogger process that sent a log packet. */
#define LOGGER_PID_T u_int32_t


/* The type used to contain a log packet serial number. */
#define LOG_SERIAL_T u_int32_t


/* Log packet format is: [LOG_SERIAL_T logger_pid][LOGGER_PID_T log_serial][zlib data]. */
#define LOG_HEADER_SIZE (sizeof(LOGGER_PID_T) + sizeof(LOG_SERIAL_T))
#define LOG_ZDATA_SIZE (int)((INPUT_BUFFER_SIZE * 1.1) + 12)
#define LOG_PACKET_SIZE (LOG_HEADER_SIZE + LOG_ZDATA_SIZE)


/* Beacon packet format is: [BEACON_STRING]. */
#define BEACON_PACKET_SIZE 32
#define BEACON_STRING "UDPLOGGER BEACON"


/* Global Variables (see udplogger.c). */
extern struct udplogger_configuration_t conf;
extern struct log_target_t *targets;
extern pthread_mutex_t targets_mutex;


#endif
