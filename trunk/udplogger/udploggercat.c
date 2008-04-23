#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "udplogger.h"


/*
 * Give a strnlen prototype to stop compiler warnings.  GNU libraries give it to us,
 * but ISO C90 doesn't allow us to use _GNU_SOURCE before including <string.h>.
 */
size_t strnlen(const char *, size_t);


/*
 * Structure that contains configuration information for the running instance
 * of this udploggercat client program.
 */
struct udploggercat_configuration_t {
	unsigned char delimiter_character;
} udploggercat_conf;


/*
 * The size of the memory to allocate for the time string buffer, which stores
 * the current timestamp in a string for easy printing.
 */
#define TIME_STRING_BUFFER_SIZE 40U


/**
 * log_packet_hook(<source host>, <log line>)
 *
 * Trivial implementation of log_packet_hook for linking with udploggerclientlib.o.
 * Prints a timestamp, the source host information, and then the log data itself.
 **/
void inline log_packet_hook(struct sockaddr_in *sender, char *line)
{
	static time_t current_timestamp = 0;
	static struct tm current_time;
	static char current_time_str[TIME_STRING_BUFFER_SIZE];
	int i;

	if ((! current_timestamp) || (current_timestamp != time(NULL)))
	{
		current_timestamp = time(NULL);
		if (current_timestamp == (time_t)-1)
		{
			perror("udploggercat.c time()");
			return;
		}
		if (localtime_r(&current_timestamp, &current_time) == NULL)
		{
			perror("udploggercat.c localtime_r()");
			return;
		}
		strftime(current_time_str, TIME_STRING_BUFFER_SIZE, "[%Y-%m-%d %H:%M:%S]", &current_time);
		current_time_str[TIME_STRING_BUFFER_SIZE - 1] = '\0';
	}
	if (udploggercat_conf.delimiter_character != DELIMITER_CHARACTER)
	{
		for (i = 0; i < strnlen(line, PACKET_MAXIMUM_SIZE); i++)
		{
			if (line[i] == DELIMITER_CHARACTER)
			{
				line[i] = udploggercat_conf.delimiter_character;
			}
		}
	}
	printf("%s%c[%s:%hu]%c%s\n", current_time_str, udploggercat_conf.delimiter_character, inet_ntoa(sender->sin_addr), ntohs(sender->sin_port), udploggercat_conf.delimiter_character, line);
}
