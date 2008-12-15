#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include "udplogger.h"
#include "udploggerparselib.h"


/*
 * Please note that this file cannot link successfully by itself.  It is useful as a part of the udplogger
 * client libraries.  This library implements parsing functionality that can be used to translate a raw-data
 * log line into a structure that can be easily used throughout code.
 *
 * This library consists of a main parse_log_line function definition as well as a help parse_* function for
 * each field of the log line.  The main parse_log_line function satisfies the prototype of
 * parse_log_line as defined in udploggerclientlib.h.  Each helper function is called for the field that matches
 * it's index in the parse_functions array.
 *
 * For fields that map static strings to enums, the parse_* function simply calls a map_* function to perform the
 * static string -> enum mapping.  This is done so that the map_* functions can be used in other places in the
 * codebase (i.e. parsing command-line parameter arguments).
 *
 * See 'udploggerc.c' for a simple example.
 */


/*
 * Give a strnlen prototype to stop compiler warnings.  GNU libraries give it to us,
 * but ISO C90 doesn't allow us to use _GNU_SOURCE before including <string.h>.
 */
size_t strnlen(const char *, size_t);


void parse_body_size(const char *, struct log_entry_t *);
void parse_bytes_incoming(const char *, struct log_entry_t *);
void parse_bytes_outgoing(const char *, struct log_entry_t *);
void parse_connection_status(const char *, struct log_entry_t *);
void parse_forwarded_for(const char *, struct log_entry_t *);
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
	void (*parse_functions[])(const char *, struct log_entry_t *) = {
		&parse_timestamp,
		&parse_source,
		&parse_serial,
		&parse_tag,
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
	char *ptr;
	char tmp[PACKET_MAXIMUM_SIZE];

	memcpy(tmp, line, PACKET_MAXIMUM_SIZE);
	length = strnlen(tmp, PACKET_MAXIMUM_SIZE);
	for (i = 0; i < length; i++)
	{
		if (tmp[i] == DELIMITER_CHARACTER)
		{
			tmp[i] = '\0';
		}
	}

	ptr = tmp;
	i = 0;
	while ((ptr < (tmp + length)) && (parse_functions[i] != NULL))
	{
		(*parse_functions[i])(ptr, data);
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
	printf("udploggerclientlib.c debug:    parse_body_size('%s') => %u\n", field, data->body_size);
#endif
}


void parse_bytes_incoming(const char *field, struct log_entry_t *data)
{
	if (! sscanf(field, "%u", &(data->bytes_incoming)))
	{
		data->bytes_incoming = 0;
	}
#ifdef __DEBUG__
	printf("udploggerclientlib.c debug:    parse_bytes_incoming('%s') => %u\n", field, data->bytes_incoming);
#endif
}


void parse_bytes_outgoing(const char *field, struct log_entry_t *data)
{
	if (! sscanf(field, "%u", &(data->bytes_outgoing)))
	{
		data->bytes_outgoing = 0;
	}
#ifdef __DEBUG__
	printf("udploggerclientlib.c debug:    parse_bytes_outgoing('%s') => %u\n", field, data->bytes_outgoing);
#endif
}


void parse_connection_status(const char *field, struct log_entry_t *data)
{
	data->connection_status = map_connection_status(field);
#ifdef __DEBUG__
	printf("udploggerclientlib.c debug:    parse_connection_status('%s') => %u\n", field, data->connection_status);
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
	printf("udploggerclientlib.c debug:    parse_forwarded_for('%s') => '%s'\n", field, data->forwarded_for);
#endif
}


void parse_method(const char *field, struct log_entry_t *data)
{
	data->method = map_method(field);
#ifdef __DEBUG__
	printf("udploggerclientlib.c debug:    parse_method('%s') => %u\n", field, data->method);
#endif
}


void parse_nexopia_userage(const char *field, struct log_entry_t *data)
{
	if (! sscanf(field, "%hu", &(data->nexopia_userage)))
	{
		data->nexopia_userage = 0;
	}
#ifdef __DEBUG__
	printf("udploggerclientlib.c debug:    parse_nexopia_userage('%s') => %hu\n", field, data->nexopia_userage);
#endif
}


void parse_nexopia_userid(const char *field, struct log_entry_t *data)
{
	if (! sscanf(field, "%lu", &(data->nexopia_userid)))
	{
		data->nexopia_userid = 0;
	}
#ifdef __DEBUG__
	printf("udploggerclientlib.c debug:    parse_nexopia_userid('%s') => %lu\n", field, data->nexopia_userid);
#endif
}


void parse_nexopia_userlocation(const char *field, struct log_entry_t *data)
{
	if (! sscanf(field, "%lu", &(data->nexopia_userlocation)))
	{
		data->nexopia_userlocation = 0;
	}
#ifdef __DEBUG__
	printf("udploggerclientlib.c debug:    parse_nexopia_userlocation('%s') => %lu\n", field, data->nexopia_userlocation);
#endif
}


void parse_nexopia_usersex(const char *field, struct log_entry_t *data)
{
	data->nexopia_usersex = map_nexopia_usersex(field);
#ifdef __DEBUG__
	printf("udploggerclientlib.c debug:    parse_nexopia_usersex('%s') => %u\n", field, data->nexopia_usersex);
#endif
}


void parse_nexopia_usertype(const char *field, struct log_entry_t *data)
{
	data->nexopia_usertype = map_nexopia_usertype(field);
#ifdef __DEBUG__
	printf("udploggerclientlib.c debug:    parse_nexopia_usertype('%s') => %u\n", field, data->nexopia_usertype);
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
	printf("udploggerclientlib.c debug:    parse_query_string('%s') => '%s'\n", field, data->query_string);
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
	printf("udploggerclientlib.c debug:    parse_referer('%s') => '%s'\n", field, data->referer);
#endif
}


void parse_remote_address(const char *field, struct log_entry_t *data)
{
	if (! inet_aton(field, &(data->remote_address)))
	{
		bzero(&(data->remote_address), sizeof(data->remote_address));
	}
#ifdef __DEBUG__
	printf("udploggerclientlib.c debug:    parse_remote_address('%s') => %s\n", field, inet_ntoa(data->remote_address));
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
	printf("udploggerclientlib.c debug:    parse_request_url('%s') => '%s'\n", field, data->request_url);
#endif
}


void parse_serial(const char *field, struct log_entry_t *data)
{
	if (! sscanf(field, "%lu", &(data->serial)))
	{
		data->serial = 0;
	}
#ifdef __DEBUG__
	printf("udploggerclientlib.c debug:    parse_serial('%s') => %lu\n", field, data->serial);
#endif
}


void parse_status(const char *field, struct log_entry_t *data)
{
	if (! sscanf(field, "%hu", &(data->status)))
	{
		data->status = 0;
	}
#ifdef __DEBUG__
	printf("udploggerclientlib.c debug:    parse_status('%s') => %hu\n", field, data->status);
#endif
}


void parse_tag(const char *field, struct log_entry_t *data)
{
	memcpy(&(data->tag), field, strlen(field) + 1);
#ifdef __DEBUG__
	printf("udploggerclientlib.c debug:    parse_tag('%s') => '%s'\n", field, data->tag);
#endif
}


void parse_time_used(const char *field, struct log_entry_t *data)
{
	if (! sscanf(field, "%hu", &(data->time_used)))
	{
		data->time_used = 0;
	}
#ifdef __DEBUG__
	printf("udploggerclientlib.c debug:    parse_time_used('%s') => %hu\n", field, data->time_used);
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
	printf("udploggerclientlib.c debug:    parse_user_agent('%s') => '%s'\n", field, data->user_agent);
#endif
}
