#ifndef __UDPLOGGERSTATS_H__
#define __UDPLOGGERSTATS_H__

#include "udplogger.h"

#define HIT_MAX_HEADERS		32
#define HIT_MAX_HDR_LENGTH	64

#define PARSE_OK	0
#define PARSE_ERR	1

typedef struct {
	char *key;
	char *value;
} kvlist_t;

typedef struct {
	LOGGER_PID_T		pid;
	LOGGER_CNT_T		cnt;
	struct sockaddr_in	*addr;

	/**
	 * All of the raw_* variables are strings
	 **/
	char *raw_remote_host;		// %h
	char *raw_auth_user;		// %u
	char *raw_request_start;	// %t
	char *raw_request_line;		// %r
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

	kvlist_t headers[HIT_MAX_HEADERS];	// %{}[io]

	unsigned char parse_status;
} hit_t;

#endif //!__UDPLOGGERSTATS_H__
