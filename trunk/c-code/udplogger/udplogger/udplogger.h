#ifndef __UDPLOGGER_H__
#define __UDPLOGGER_H__

struct udplogger_configuration_t {
	uint8_t compress_level;
	uint16_t listen_port;
	uintmax_t maximum_target_age;
	long prune_target_maximum_interval;
};

#define INPUT_BUFFER_SIZE 1024
#define LOG_SERIAL_T u_int32_t
#define LOGGER_PID_T u_int32_t

/**
 * Packet Descriptors
 * 
 * Packet format is: [u_int32_t logger_pid][u_int32_t log_serial][zlib data]
 **/
#define LOG_HEADER_SIZE (sizeof(LOGGER_PID_T) + sizeof(LOG_SERIAL_T))
#define LOG_ZDATA_SIZE (int)((INPUT_BUFFER_SIZE * 1.1) + 12)
#define LOG_PACKET_SIZE (LOG_HEADER_SIZE + LOG_ZDATA_SIZE)

// Beacon Identifier String
#define BEACON_STRING "UDP LOGGER BEACON"

// Function Prototypes
int arguments_parse(int, char **);
void arguments_show_usage();
int bind_socket(uint16_t);

/**
 * Global Variables (variables shared across threads).
 **/
struct udplogger_configuration_t conf; // Our current configuration (defaults over-ridden by command-line parameters).
struct log_target *targets = NULL;     // A singly-linked list of the log targets that we should report to.
pthread_mutex_t targets_mutex;         // The mutex controlling access to the list of log targets.

/**
 * Default configuration parameters.
 **/
#define DEFAULT_COMPRESS_LEVEL 0
#define DEFAULT_LISTEN_PORT 43824U
#define DEFAULT_MAXIMUM_TARGET_AGE 120UL
#define DEFAULT_PRUNE_TARGET_MAXIMUM_INTERVAL 10L

#endif //!__UDPLOGGER_H__
