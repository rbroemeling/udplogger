#ifndef __UDPLOGGER_H__
#define __UDPLOGGER_H__

#define DEFAULT_COMPRESS_LEVEL 0
#define DEFAULT_LISTEN_PORT 43824U
#define DEFAULT_MAXIMUM_TARGET_AGE 120UL

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

/**
 * Beacon Identifier String
 **/
#define BEACON_STRING "UDP LOGGER BEACON"

int arguments_parse(int, char **);
void arguments_show_usage();
int bind_socket(uint16_t);

#endif //!__UDPLOGGER_H__
