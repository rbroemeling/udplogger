#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "udplogger.h"
#include "udploggerparselib.h"


/*
 * Please note that this file cannot link successfully by itself.  It is useful as a part of the udplogger
 * data libraries.  This library implements parsing functionality that can be used to translate a raw-data
 * log line into a structure that can be easily used throughout code.
 *
 * This library consists of a main parse_log_line function definition as well as a help parse_* function for
 * each field of the log line.  The main parse_log_line function satisfies the prototype of
 * parse_log_line as defined in udploggerparselib.h.  Each helper function is called for the field that matches
 * it's index in the parse function arrays.
 *
 * For fields that map static strings to enums, the parse_* function simply calls a map_* function to perform the
 * static string -> enum mapping.  This is done so that the map_* functions can be used in other places in the
 * codebase (i.e. parsing command-line parameter arguments).
 *
 * See 'udploggergrep.cc' for a simple example.
 */


void parse_body_size(const char *, struct log_entry_t *);
void parse_bytes_incoming(const char *, struct log_entry_t *);
void parse_bytes_outgoing(const char *, struct log_entry_t *);
void parse_connection_status(const char *, struct log_entry_t *);
void parse_content_type(const char *, struct log_entry_t *);
void parse_forwarded_for(const char *, struct log_entry_t *);
void parse_host(const char *, struct log_entry_t *);
void parse_method(const char *, struct log_entry_t *);
void parse_nexopia_userage(const char *, struct log_entry_t *);
void parse_nexopia_userid(const char *, struct log_entry_t *);
void parse_nexopia_userlocation(const char *, struct log_entry_t *);
void parse_nexopia_usersex(const char *, struct log_entry_t *);
void parse_nexopia_usertype(const char *, struct log_entry_t *);
void parse_query_string(const char *, struct log_entry_t *);
void parse_referer(const char *, struct log_entry_t *);
void parse_remote_address(const char *, struct log_entry_t *);
void parse_request_url(const char *, struct log_entry_t *);
void parse_serial(const char *, struct log_entry_t *);
void parse_source(const char *, struct log_entry_t *);
void parse_status(const char *, struct log_entry_t *);
void parse_tag(const char *, struct log_entry_t *);
void parse_time_used(const char *, struct log_entry_t *);
void parse_timestamp(const char*, struct log_entry_t *);
void parse_user_agent(const char *, struct log_entry_t *);
void parse_version(const char *, struct log_entry_t *);


/**
 * parse_log_line(line, data)
 *
 * Copies the log line to a temporary buffer and then null-delimits the fields.
 * Then iterates through the fields and calls the matching parse_* function for each field.
 **/
void parse_log_line(char *line, struct log_entry_t *data)
{
	int length;
	int i;

	/**
	 * Base parse functions -- these functions parse all the fields that are
	 * present before the web-server's (versioned) web-log/access fields.
	 **/
	void (*parse_functions[])(const char *, struct log_entry_t *) = {
		&parse_timestamp,
		&parse_source,
		&parse_serial,
		&parse_tag,
		NULL
	};

	/**
	 * v1 parse functions -- this is the parse order for version 1 of the
	 * web-server's fields.  The fields are representative of version 1 if
	 * the first field does not match 'v<VERSION NUMBER>'.
	 **/
	void (*parse_functions_v1[])(const char *, struct log_entry_t *) = {
		&parse_method,
		&parse_status,
		&parse_body_size,
		&parse_bytes_incoming,
		&parse_bytes_outgoing,
		&parse_time_used,
		&parse_connection_status,
		&parse_request_url,
		&parse_query_string,
		&parse_remote_address,
		&parse_user_agent,
		&parse_forwarded_for,
		&parse_referer,
		&parse_nexopia_userid,
		&parse_nexopia_userage,
		&parse_nexopia_usersex,
		&parse_nexopia_userlocation,
		&parse_nexopia_usertype,
		NULL
	};

	/**
	 * v2 parse functions -- this is the parse order for version 2 of the
	 * web-server's fields.  The fields are representative of version 2 if
	 * the first field is 'v2'.
	 **/
	void (*parse_functions_v2[])(const char *, struct log_entry_t *) = {
		&parse_version,
		&parse_method,
		&parse_status,
		&parse_body_size,
		&parse_bytes_incoming,
		&parse_bytes_outgoing,
		&parse_time_used,
		&parse_connection_status,
		&parse_request_url,
		&parse_query_string,
		&parse_remote_address,
		&parse_host,
		&parse_user_agent,
		&parse_forwarded_for,
		&parse_referer,
		&parse_content_type,
		&parse_nexopia_userid,
		&parse_nexopia_userage,
		&parse_nexopia_usersex,
		&parse_nexopia_userlocation,
		&parse_nexopia_usertype,
		NULL
	};
	void (**pfunc)(const char *, struct log_entry_t *);
	char *ptr;
	char tmp[PACKET_MAXIMUM_SIZE];

	memcpy(tmp, line, PACKET_MAXIMUM_SIZE);
	length = strnlen(tmp, PACKET_MAXIMUM_SIZE);
	for (i = 0; i < length; i++)
	{
		if ((tmp[i] == DELIMITER_CHARACTER) || (tmp[i] == '\n'))
		{
			tmp[i] = '\0';
		}
	}

	/* Parse the first fields, those that are basic to udplogger functionality. */
	pfunc = parse_functions;
	ptr = tmp;
	i = 0;
	while ((ptr < (tmp + length)) && (pfunc[i] != NULL))
	{
		(*pfunc[i])(ptr, data);
		i++;
		while ((ptr < (tmp + length)) && (*ptr != '\0'))
		{
			ptr++;
		}
		ptr++;
	}

	/* Begin versioned parsing by checking the current field. */
	if (! strcasecmp(ptr, "v2"))
	{
		pfunc = parse_functions_v2;
	}
	else
	{
		data->content_type[0] = '\0';
		data->host[0] = '\0';
		data->version = 1;
		pfunc = parse_functions_v1;
	}

	i = 0;
	while ((ptr < (tmp + length)) && (pfunc[i] != NULL))
	{
		(*pfunc[i])(ptr, data);
		i++;
		while ((ptr < (tmp + length)) && (*ptr != '\0'))
		{
			ptr++;
		}
		ptr++;
	}
}


enum connection_status_enum map_connection_status(const char *field)
{
	if (! strcasecmp(field, "X"))
	{
		return connection_status_aborted;
	}
	else if (! strcasecmp(field, "+"))
	{
		return connection_status_keep_alive;
	}
	else if (! strcasecmp(field, "-"))
	{
		return connection_status_close;
	}
	return connection_status_unknown;
}


enum request_method_enum map_method(const char *field)
{
	if (! strcasecmp(field, "get"))
	{
		return request_method_get;
	}
	else if (! strcasecmp(field, "post"))
	{
		return request_method_post;
	}
	else if (! strcasecmp(field, "head"))
	{
		return request_method_head;
	}
	else if (! strcasecmp(field, "options"))
	{
		return request_method_options;
	}
	else if (! strcasecmp(field, "put"))
	{
		return request_method_put;
	}
	else if (! strcasecmp(field, "delete"))
	{
		return request_method_delete;
	}
	else if (! strcasecmp(field, "trace"))
	{
		return request_method_trace;
	}
	else if (! strcasecmp(field, "connect"))
	{
		return request_method_connect;
	}
	return request_method_unknown;
}

enum sex_enum map_nexopia_usersex(const char *field)
{
	if (! strcasecmp(field, "female"))
	{
		return sex_female;
	}
	else if (! strcasecmp(field, "male"))
	{
		return sex_male;
	}
	return sex_unknown;
}

enum usertype_enum map_nexopia_usertype(const char *field)
{
	if (! strcasecmp(field, "anon"))
	{
		return usertype_anon;
	}
	else if (! strcasecmp(field, "user"))
	{
		return usertype_user;
	}
	else if (! strcasecmp(field, "plus"))
	{
		return usertype_plus;
	}
	return usertype_unknown;
}


void parse_body_size(const char *field, struct log_entry_t *data)
{
	if (! sscanf(field, "%u", &(data->body_size)))
	{
		data->body_size = 0;
	}
#ifdef __DEBUG__
	printf("udploggerparselib.cc debug:    parse_body_size('%s') => %u\n", field, data->body_size);
#endif
}


void parse_bytes_incoming(const char *field, struct log_entry_t *data)
{
	if (! sscanf(field, "%u", &(data->bytes_incoming)))
	{
		data->bytes_incoming = 0;
	}
#ifdef __DEBUG__
	printf("udploggerparselib.cc debug:    parse_bytes_incoming('%s') => %u\n", field, data->bytes_incoming);
#endif
}


void parse_bytes_outgoing(const char *field, struct log_entry_t *data)
{
	if (! sscanf(field, "%u", &(data->bytes_outgoing)))
	{
		data->bytes_outgoing = 0;
	}
#ifdef __DEBUG__
	printf("udploggerparselib.cc debug:    parse_bytes_outgoing('%s') => %u\n", field, data->bytes_outgoing);
#endif
}


void parse_connection_status(const char *field, struct log_entry_t *data)
{
	data->connection_status = map_connection_status(field);
#ifdef __DEBUG__
	printf("udploggerparselib.cc debug:    parse_connection_status('%s') => %u\n", field, data->connection_status);
#endif
}


void parse_content_type(const char *field, struct log_entry_t *data)
{
	if (! strcasecmp(field, "-"))
	{
		data->content_type[0] = '\0';
	}
	else
	{
		memcpy(&(data->content_type), field, strlen(field) + 1);
	}
#ifdef __DEBUG__
	printf("udploggerparselib.cc debug:    parse_content_type('%s') => '%s'\n", field, data->content_type);
#endif
}


void parse_forwarded_for(const char *field, struct log_entry_t *data)
{
	if (! strcasecmp(field, "-"))
	{
		data->forwarded_for[0] = '\0';
	}
	else
	{
		memcpy(&(data->forwarded_for), field, strlen(field) + 1);
	}
#ifdef __DEBUG__
	printf("udploggerparselib.cc debug:    parse_forwarded_for('%s') => '%s'\n", field, data->forwarded_for);
#endif
}


void parse_host(const char *field, struct log_entry_t *data)
{
	if (! strcasecmp(field, "-"))
	{
		data->host[0] = '\0';
	}
	else
	{
		memcpy(&(data->host), field, strlen(field) + 1);
	}
#ifdef __DEBUG__
	printf("udploggerparselib.cc debug:    parse_host('%s') => '%s'\n", field, data->host);
#endif
}


void parse_method(const char *field, struct log_entry_t *data)
{
	data->method = map_method(field);
#ifdef __DEBUG__
	printf("udploggerparselib.cc debug:    parse_method('%s') => %u\n", field, data->method);
#endif
}


void parse_nexopia_userage(const char *field, struct log_entry_t *data)
{
	if (! sscanf(field, "%hu", &(data->nexopia_userage)))
	{
		data->nexopia_userage = 0;
	}
#ifdef __DEBUG__
	printf("udploggerparselib.cc debug:    parse_nexopia_userage('%s') => %hu\n", field, data->nexopia_userage);
#endif
}


void parse_nexopia_userid(const char *field, struct log_entry_t *data)
{
	if (! sscanf(field, "%lu", &(data->nexopia_userid)))
	{
		data->nexopia_userid = 0;
	}
#ifdef __DEBUG__
	printf("udploggerparselib.cc debug:    parse_nexopia_userid('%s') => %lu\n", field, data->nexopia_userid);
#endif
}


void parse_nexopia_userlocation(const char *field, struct log_entry_t *data)
{
	if (! sscanf(field, "%lu", &(data->nexopia_userlocation)))
	{
		data->nexopia_userlocation = 0;
	}
#ifdef __DEBUG__
	printf("udploggerparselib.cc debug:    parse_nexopia_userlocation('%s') => %lu\n", field, data->nexopia_userlocation);
#endif
}


void parse_nexopia_usersex(const char *field, struct log_entry_t *data)
{
	data->nexopia_usersex = map_nexopia_usersex(field);
#ifdef __DEBUG__
	printf("udploggerparselib.cc debug:    parse_nexopia_usersex('%s') => %u\n", field, data->nexopia_usersex);
#endif
}


void parse_nexopia_usertype(const char *field, struct log_entry_t *data)
{
	data->nexopia_usertype = map_nexopia_usertype(field);
#ifdef __DEBUG__
	printf("udploggerparselib.cc debug:    parse_nexopia_usertype('%s') => %u\n", field, data->nexopia_usertype);
#endif
}


void parse_query_string(const char *field, struct log_entry_t *data)
{
	if (! strcasecmp(field, "-"))
	{
		data->query_string[0] = '\0';
	}
	else
	{
		memcpy(&(data->query_string), field, strlen(field) + 1);
	}
#ifdef __DEBUG__
	printf("udploggerparselib.cc debug:    parse_query_string('%s') => '%s'\n", field, data->query_string);
#endif
}


void parse_referer(const char *field, struct log_entry_t *data)
{
	if (! strcasecmp(field, "-"))
	{
		data->referer[0] = '\0';
	}
	else
	{
		memcpy(&(data->referer), field, strlen(field) + 1);
	}
#ifdef __DEBUG__
	printf("udploggerparselib.cc debug:    parse_referer('%s') => '%s'\n", field, data->referer);
#endif
}


void parse_remote_address(const char *field, struct log_entry_t *data)
{
	if (! inet_aton(field, &(data->remote_address)))
	{
		bzero(&(data->remote_address), sizeof(data->remote_address));
	}
#ifdef __DEBUG__
	printf("udploggerparselib.cc debug:    parse_remote_address('%s') => %s\n", field, inet_ntoa(data->remote_address));
#endif
}


void parse_request_url(const char *field, struct log_entry_t *data)
{
	if (! strcasecmp(field, "-"))
	{
		data->request_url[0] = '\0';
	}
	else
	{
		memcpy(&(data->request_url), field, strlen(field) + 1);
	}
#ifdef __DEBUG__
	printf("udploggerparselib.cc debug:    parse_request_url('%s') => '%s'\n", field, data->request_url);
#endif
}


void parse_serial(const char *field, struct log_entry_t *data)
{
	if (! sscanf(field, "%lu", &(data->serial)))
	{
		data->serial = 0;
	}
#ifdef __DEBUG__
	printf("udploggerparselib.cc debug:    parse_serial('%s') => %lu\n", field, data->serial);
#endif
}


void parse_source(const char *field, struct log_entry_t *data)
{
	/* 16 is the maximum length (in characters) of an IPv4 address (255.255.255.255 is
	 * 15 characters) plus a terminating null byte ('\0'). */
	char address[16];

	switch (sscanf(field, "[%15[0-9.]:%hu]", address, &(data->source_port)))
	{
		case 1:
			data->source_port = 0;
		case 2:
			if (! inet_aton(address, &(data->source_address)))
			{
				bzero(&(data->source_address), sizeof(data->source_address));
			}
			break;
		default:
			bzero(&(data->source_address), sizeof(data->source_address));
			data->source_port = 0;
			break;
	}
#ifdef __DEBUG__
	printf("udploggerparselib.cc debug:    parse_source('%s') => [%s:%hu]\n", field, inet_ntoa(data->source_address), data->source_port);
#endif
}


void parse_status(const char *field, struct log_entry_t *data)
{
	if (! sscanf(field, "%hu", &(data->status)))
	{
		data->status = 0;
	}
#ifdef __DEBUG__
	printf("udploggerparselib.cc debug:    parse_status('%s') => %hu\n", field, data->status);
#endif
}


void parse_tag(const char *field, struct log_entry_t *data)
{
	memcpy(&(data->tag), field, strlen(field) + 1);
#ifdef __DEBUG__
	printf("udploggerparselib.cc debug:    parse_tag('%s') => '%s'\n", field, data->tag);
#endif
}


void parse_time_used(const char *field, struct log_entry_t *data)
{
	if (! sscanf(field, "%hu", &(data->time_used)))
	{
		data->time_used = 0;
	}
#ifdef __DEBUG__
	printf("udploggerparselib.cc debug:    parse_time_used('%s') => %hu\n", field, data->time_used);
#endif
}


void parse_timestamp(const char *field, struct log_entry_t *data)
{
#ifdef __DEBUG__
	/* 32 characters is a comfortable buffer to store a normal date string
	 * ([YYYY-mm-dd HH:MM:SS], 21 characters in length). */
	#define TIMESTAMP_BUFFER_SIZE 32U
	char debug_timestamp_str[TIMESTAMP_BUFFER_SIZE];
#endif
	char *ptr;

	bzero(&(data->timestamp), sizeof(data->timestamp));
	ptr = strptime(field, "[%Y-%m-%d %H:%M:%S]", &(data->timestamp));
	if (ptr == NULL || *ptr != '\0')
	{
		bzero(&(data->timestamp), sizeof(data->timestamp));
	}
#ifdef __DEBUG__
	strftime(debug_timestamp_str, TIMESTAMP_BUFFER_SIZE, "[%Y-%m-%d %H:%M:%S]", &(data->timestamp));
	debug_timestamp_str[TIMESTAMP_BUFFER_SIZE - 1] = '\0';
	printf("udploggerparselib.cc debug:    parse_timestamp('%s') => %s\n", field, debug_timestamp_str);
	printf("                                 tm_sec: %d\n", data->timestamp.tm_sec);
	printf("                                 tm_min: %d\n", data->timestamp.tm_min);
	printf("                                 tm_hour: %d\n", data->timestamp.tm_hour);
	printf("                                 tm_mday: %d\n", data->timestamp.tm_mday);
	printf("                                 tm_mon: %d\n", data->timestamp.tm_mon);
	printf("                                 tm_year: %d\n", data->timestamp.tm_year);
	printf("                                 tm_wday: %d\n", data->timestamp.tm_wday);
	printf("                                 tm_yday: %d\n", data->timestamp.tm_yday);
	printf("                                 tm_isdst: %d\n", data->timestamp.tm_isdst);
#endif
}


void parse_user_agent(const char *field, struct log_entry_t *data)
{
	if (! strcasecmp(field, "-"))
	{
		data->user_agent[0] = '\0';
	}
	else
	{
		memcpy(&(data->user_agent), field, strlen(field) + 1);
	}
#ifdef __DEBUG__
	printf("udploggerparselib.cc debug:    parse_user_agent('%s') => '%s'\n", field, data->user_agent);
#endif
}


void parse_version(const char *field, struct log_entry_t *data)
{
	if ((! sscanf(field, "v%hu", &(data->version))) && (! sscanf(field, "V%hu", &(data->version))))
	{
		data->version = 0;
	}
#ifdef __DEBUG__
	printf("udploggerparselib.cc debug:    parse_version('%s') => %hu\n", field, data->version);
#endif
}