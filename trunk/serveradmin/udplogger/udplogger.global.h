#ifndef __UDPLOGGER_GLOBAL_H__
#define __UDPLOGGER_GLOBAL_H__

/* Beacon packet format is: [beacon string]. */
#define BEACON_PACKET_SIZE 32
#define BEACON_STRING "UDPLOGGER BEACON"

/* The maximum length of a single log line (as read from stdin). */
#define INPUT_BUFFER_SIZE (1024 * 8)

/* Log packet format is:     [serial]   [tag]      [log data]. */
#define PACKET_MAXIMUM_SIZE ((20 + 1) + (10 + 1) + INPUT_BUFFER_SIZE)

/* The default port that udplogger will use to communicate. */
#define UDPLOGGER_DEFAULT_PORT                   43824U

#endif
