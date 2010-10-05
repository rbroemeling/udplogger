#ifndef __UDPLOGGER_H__
#define __UDPLOGGER_H__

/* 
 * Log data format is:
 *   accesslog.format = "%m%s%b%I%O%T%X%U%q%a%{User-agent}i%{X-Forwarded-For}i%{Referer}i"
 * Or:
 *   accesslog.format = "%m%s%b%I%O%T%X%U%q%a%{User-agent}i%{X-Forwarded-For}i%{Referer}i%{X-LIGHTTPD-userid}o%{X-LIGHTTPD-age}o%{X-LIGHTTPD-sex}o%{X-LIGHTTPD-loc}o%{X-LIGHTTPD-usertype}o"
 */
 
/* Beacon packet format is: [beacon string]. */
#define BEACON_PACKET_SIZE 32
#define BEACON_STRING "UDPLOGGER BEACON"

/* The character to be used to delimit log fields.  We use U+001E, decimal 30, octal 036 (the information/record separator character). */
#define DELIMITER_CHARACTER 30U

/* The maximum length of a single log line (as read from stdin). */
#define INPUT_BUFFER_SIZE (1024 * 8)

/* Log packet format is:     [serial]   [tag]      [log data]. */
#define PACKET_MAXIMUM_SIZE ((20 + 1) + (10 + 1) + INPUT_BUFFER_SIZE)

/* The maximum length (characters) that a log line tag is allowed to be. */
#define TAG_MAXIMUM_LENGTH 10

/* The default port that udplogger will use to communicate. */
#define UDPLOGGER_DEFAULT_PORT 43824U

#endif
