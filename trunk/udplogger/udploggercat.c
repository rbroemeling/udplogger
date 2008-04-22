#include <arpa/inet.h>
#include <stdio.h>
#include <time.h>


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
void inline log_packet_hook(const struct sockaddr_in *sender, const char *line)
{
	static time_t current_timestamp = 0;
	static struct tm current_time;
	static char current_time_str[TIME_STRING_BUFFER_SIZE];

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
	printf("%s [%s:%hu] %s\n", current_time_str, inet_ntoa(sender->sin_addr), ntohs(sender->sin_port), line);
}
