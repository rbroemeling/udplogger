#ifndef __UDPLOGGERSTATS_H__
#define __UDPLOGGERSTATS_H__

#include <sys/socket.h>
#include <netinet/in.h>
#include <time.h>
#include "udplogger.h"

#define HIT_MAX_HEADERS		32
#define HIT_MAX_HDR_LENGTH	64

#define PARSE_OK	0
#define PARSE_ERR	1

#define DEFAULT_TIME_FMT "%d/%b/%Y:%T %p"

typedef struct {
	char *key;
	char *value;
} kvlist_t;

typedef enum {
	HTTP_UNKNOWN_METHOD = 0,	/* Set if we don't parse %m */
	HTTP_EXTENSION_METHOD,		/* Set if it is not an RFC2616 */
	
	/* RFC2616 - HTTP Methods */
	HTTP_OPTIONS, HTTP_GET, HTTP_HEAD, HTTP_POST, HTTP_PUT, HTTP_DELETE,
	HTTP_TRACE, HTTP_CONNECT
} http_method;

typedef enum {
	HTTP_UNKNOWN_PROTOCOL = 0,	/* Set if we don't parse %H */
	HTTP_EXTENSION_PROTOCOL,
	HTTP_0_9, HTTP_1_0, HTTP_1_1
} http_protocol;

typedef struct {
	LOGGER_PID_T		pid;
	LOGGER_CNT_T		cnt;
	struct sockaddr_in	*addr;

	/**
	 * All of the raw_* variables are strings
	 **/
	char *raw_remote_host;		// %h
	char *raw_auth_user;		// %u
	char *raw_request_start;	// %t (CLF ? may want to support %{strftime}t )
	char *raw_status_code;		// %s %>s %<s
	char *raw_body_size;		// %b or %B
	char *raw_remote_address;	// %a
	char *raw_local_address;	// %A
	char *raw_filename;			// %f
	char *raw_protocol;			// %H
	char *raw_method;			// %m
	char *raw_server_port;		// %p
	char *raw_query;			// %q
	char *raw_request_time;		// %T
	char *raw_request_url;		// %U
	char *raw_server_name;		// %v
	char *raw_request_host;		// %V
	char *raw_conn_status;		// %x
	char *raw_bytes_in;			// %I
	char *raw_bytes_out;		// %O

	kvlist_t headers[HIT_MAX_HEADERS];	// %{Foo-Bar}[ion]

	/**
	 * These are the converted versions of some values
	 **/
	http_method			method;
	http_protocol		protocol;
	in_addr_t			local_address;
	in_addr_t			remote_address;
	unsigned int		body_size;
	unsigned short		server_port;
	unsigned int		request_time;
	unsigned int		bytes_in;
	unsigned int		bytes_out;
	time_t				timestamp;
	struct tm			time;
} hit_t;

#endif //!__UDPLOGGERSTATS_H__
