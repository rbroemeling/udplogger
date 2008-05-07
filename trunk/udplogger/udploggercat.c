#include <arpa/inet.h>
#include <getopt.h>
#include <pcre.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "udplogger.h"
#include "udploggerclientlib.h"


/*
 * Give a strnlen prototype to stop compiler warnings.  GNU libraries give it to us,
 * but ISO C90 doesn't allow us to use _GNU_SOURCE before including <string.h>.
 */
size_t strnlen(const char *, size_t);


/*
 * Number of elements to use in the ovector for pcre result substrings.
 * Should be a multiple of 3.
 */
#define OVECCOUNT 3


/*
 * Structure that contains configuration information for the running instance
 * of this udploggercat client program.
 */
struct udploggercat_configuration_t {
	unsigned char delimiter_character;
	pcre *request_url_filter;
} udploggercat_conf;


/*
 * The size of the memory to allocate for the time string buffer, which stores
 * the current timestamp in a string for easy printing.
 */
#define TIME_STRING_BUFFER_SIZE 40U


int add_option_hook();
int getopt_hook(char);
void inline log_packet_hook(struct sockaddr_in *, char *);
void usage_hook();


int add_option_hook()
{
	udploggercat_conf.delimiter_character = DELIMITER_CHARACTER;
	udploggercat_conf.request_url_filter = NULL;
	return (
		add_option("delimiter", required_argument, 'd') &&
		add_option("request_url_filter", required_argument, 'u')
		);
}


int getopt_hook(char i)
{
	const char *pcre_error;
	int pcre_error_offset;

	switch (i)
	{
		case 'd':
			if (strlen(optarg) != 1)
			{
				fprintf(stderr, "udploggercat.c invalid delimiter '%s'\n", optarg);
				return -1;
			}
			else
			{
				udploggercat_conf.delimiter_character = optarg[0];
#ifdef __DEBUG__
				printf("udploggercat.c debug: setting delimiter to '%c' (0x%x)\n", udploggercat_conf.delimiter_character, udploggercat_conf.delimiter_character);
#endif
				return 1;
			}			
		case 'u':
			udploggercat_conf.request_url_filter = pcre_compile(optarg, 0, &pcre_error, &pcre_error_offset, NULL);
			if (udploggercat_conf.request_url_filter == NULL)
			{
				fprintf(stderr, "udploggercat.c invalid request_url_filter pattern at offset %d: %s\n", pcre_error_offset, pcre_error);
				return -1;
			}
			else
			{
#ifdef __DEBUG__
				printf("udploggercat.c debug: setting request_url_filter to '%s'\n", optarg);
#endif
				return 1;
			}
	}
	return 0;
}


void inline log_packet_hook(struct sockaddr_in *sender, char *line)
{
	static time_t current_timestamp = 0;
	static struct tm current_time;
	static char current_time_str[TIME_STRING_BUFFER_SIZE];
	int i;
	static struct log_entry_t log_data;
	static int ovector[OVECCOUNT];

	/*
	 * Parse the log line into the structure log_data.  This is only used if filters are in-place, but we do it all the time
         * for the sake of simplicity.  If we run into speed issues this can be optimized out to run only when required.
	 */
	bzero(&log_data, sizeof(log_data));
	parse_log_line(line, &log_data);

	/* Filter out anything that we do not want to display. */
	if (udploggercat_conf.request_url_filter != NULL)
	{
		i = pcre_exec(udploggercat_conf.request_url_filter, NULL, log_data.request_url, strlen(log_data.request_url), 0, 0, ovector, OVECCOUNT);
		if (i < 0)
		{
			return;
		}
	}

	/* Update our timestamp string (if necessary). */
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

	/* Format the log line for output. */
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


void usage_hook()
{
	printf("  -d, --delimiter <delim>           set the delimiter to be used in-between log fields\n");
	printf("                                    (defaults to character 0x%x)\n", DELIMITER_CHARACTER);
	printf("  -u, --request_url_filter <pcre>   only display log lines whose request_url field matches <pcre>\n");
}
