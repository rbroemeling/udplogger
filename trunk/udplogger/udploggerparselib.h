#ifndef __UDPLOGGERPARSELIB_H__
#define __UDPLOGGERPARSELIB_H__


#include "udplogger.h"


enum connection_status_enum
{
	connection_status_unknown,
	connection_status_aborted,
	connection_status_keep_alive,
	connection_status_close
};


enum request_method_enum
{
	request_method_unknown,
	request_method_options,
	request_method_get,
	request_method_head,
	request_method_post,
	request_method_put,
	request_method_delete,
	request_method_trace,
	request_method_connect
};


enum sex_enum
{
	sex_unknown,
	sex_male,
	sex_female
};


enum usertype_enum
{
	usertype_unknown,
	usertype_plus,
	usertype_user,
	usertype_anon
};


/*
 * Struct that holds all of the information from a single log-line in an easy-to-access-and-use way.
 * Note that the char fields are likely _way_ larger than they need to be... this is because it is hard
 * to tell what the maximum of a particular field will be; but easy to tell what the absolute maximum can be
 * (as dictated by the maximum line length).  This makes the structure safe and dealing with it simple (limit all
 * string sizes to INPUT_BUFFER_SIZE), but is a little less memory efficient than it could be.
 *
 * KISS
 */
struct log_entry_t
{
	uint32_t body_size;                              /* %b - bytes sent for body */
	uint32_t bytes_incoming;                         /* %I - bytes incoming */
	uint32_t bytes_outgoing;                         /* %O - bytes outgoing */
	enum connection_status_enum connection_status;   /* %X - connection status */
	char forwarded_for[INPUT_BUFFER_SIZE];           /* %{X-Forwarded-For} - X-Forwarded-For string */
	enum request_method_enum method;                 /* %m - request method */
	uint16_t nexopia_userage;                        /* %{X-LIGHTTPD-age}o - nexopia user age */
	unsigned long nexopia_userid;                    /* %{X-LIGHTTPD-userid}o - nexopia UID */
	unsigned long nexopia_userlocation;              /* %{X-LIGHTTPD-loc}o - nexopia user location */
	enum sex_enum nexopia_usersex;                   /* %{X-LIGHTTPD-sex}o - nexopia user sex */
	enum usertype_enum nexopia_usertype;             /* %{X-LIGHTTPD-usertype}o */
	char query_string[INPUT_BUFFER_SIZE];            /* %q - query string */
	char raw[PACKET_MAXIMUM_SIZE];                   /* raw, unmodified logline */
	char referer[INPUT_BUFFER_SIZE];                 /* %{Referer} - referer string */
	struct in_addr remote_address;                   /* %a - remote address */
	char request_url[INPUT_BUFFER_SIZE];             /* %U - request URL */
	uint32_t sender_address;                         /* log source address */
	uint16_t sender_port;                            /* log source port */
	unsigned long serial;                            /* log serial number */
	uint16_t status;                                 /* %s - status code */
	char tag[TAG_MAXIMUM_LENGTH+1];                  /* log tag */
	time_t timestamp;                                /* parse timestamp */
	uint16_t time_used;                              /* %T - time used (seconds) */
	char user_agent[INPUT_BUFFER_SIZE];              /* %{User-agent} - user agent string */
};


/**
 * map_connection_status(<status string>)
 *
 * A simple helper function to map a string to a member of the connection_status_enum enumeration.
 **/
enum connection_status_enum map_connection_status(const char *);


/**
 * map_method(<method string>)
 *
 * A simple helper function to map a string to a member of the request_method_enum enumeration.
 **/
enum request_method_enum map_method(const char *);


/**
 * map_nexopia_usersex(<sex string>)
 *
 * A simple helper function to map a string to a member of the sex_enum enumeration.
 **/
enum sex_enum map_nexopia_usersex(const char *);


/**
 * map_nexopia_usertype(<usertype string>)
 *
 * A simple helper function to map a string to a member of the usertype_enum enumeration.
 **/
enum usertype_enum map_nexopia_usertype(const char *);


/**
 * parse_log_line(<log line>, <struct ptr>)
 *
 * Utility function implemented in udploggerclientlib.c to parse a log line out into an easy-to-use struct, both of which are
 * passed in.  Returns 1 on success or 0 on failure (i.e. invalid log line format).
 **/
extern void parse_log_line(char *, struct log_entry_t *);


#endif
