#include <arpa/inet.h>
#include <getopt.h>
#include <inttypes.h>
#include <pcre.h>
#include <signal.h>
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
	FILE *log_destination;
	char *log_destination_path;
	pcre *request_url_filter;
	uint16_t status_filter;
} udploggercat_conf;


/*
 * The size of the memory to allocate for the time string buffer, which stores
 * the current timestamp in a string for easy printing.
 */
#define TIME_STRING_BUFFER_SIZE 40U


int add_option_hook();
int getopt_hook(char);
void inline handle_signal_hook(sigset_t *);
void inline log_packet_hook(struct sockaddr_in *, char *);
void usage_hook();


int add_option_hook()
{
	udploggercat_conf.delimiter_character = DELIMITER_CHARACTER;
	udploggercat_conf.log_destination = stdout;
	udploggercat_conf.log_destination_path = NULL;
	udploggercat_conf.request_url_filter = NULL;
	udploggercat_conf.status_filter = 0;
	return
	(
		add_option("delimiter", required_argument, 'd') &&
		add_option("file", required_argument, 'f') &&
		add_option("request_url_filter", required_argument, 'u') &&
		add_option("status_filter", required_argument, 's')
	);
}


int getopt_hook(char i)
{
	const char *pcre_error;
	int pcre_error_offset;
	uintmax_t uint_tmp;

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
		case 'f':
			if (strcmp("-", optarg))
			{
				udploggercat_conf.log_destination_path = strdup(optarg);
				if (udploggercat_conf.log_destination_path == NULL)
				{
					perror("udploggercat.c strdup()");
					return -1;
				}
				udploggercat_conf.log_destination = fopen(udploggercat_conf.log_destination_path, "a");
				if (udploggercat_conf.log_destination == NULL)
				{
					perror("udploggercat.c fopen()");
					fprintf(stderr, "udploggercat.c could not open file '%s' for appending\n", udploggercat_conf.log_destination_path);
					return -1;
				}
			}
#ifdef __DEBUG__
			printf("udploggercat.c debug: setting output file to '%s'\n", udploggercat_conf.log_destination_path);
#endif
			return 1;
		case 's':
			uint_tmp = strtoumax(optarg, 0, 10);
			if (! uint_tmp || uint_tmp == UINT_MAX || uint_tmp > 65535)
			{
				fprintf(stderr, "udploggercat.c invalid status filter '%s'\n", optarg);
				return -1;
			}
			else
			{
				udploggercat_conf.status_filter = uint_tmp;
#ifdef __DEBUG__
				printf("udploggercat.c debug: setting status_filter to '%hu'\n", udploggercat_conf.status_filter);
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


void inline handle_signal_hook(sigset_t *signal_flags)
{
	FILE *reopened_log_destination = NULL;

	if (sigismember(signal_flags, SIGHUP) && (udploggercat_conf.log_destination_path != NULL))
	{
		reopened_log_destination = fopen(udploggercat_conf.log_destination_path, "a");
		if (reopened_log_destination == NULL)
		{
			perror("udploggercat.c fopen()");
			fprintf(stderr, "udploggercat.c could not open file '%s' for appending\n", udploggercat_conf.log_destination_path);
		}
		else
		{
			if (fclose(udploggercat_conf.log_destination) != 0)
			{
				perror("udploggercat.c fclose()");
			}
			udploggercat_conf.log_destination = reopened_log_destination;
		}
		reopened_log_destination = NULL;
	}
	if (sigismember(signal_flags, SIGTERM) && (udploggercat_conf.log_destination_path != NULL))
	{
		if (fclose(udploggercat_conf.log_destination) != 0)
		{
			perror("udploggercat.c fclose()");
		}
		udploggercat_conf.log_destination = NULL;
	}
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
	if (udploggercat_conf.status_filter && (udploggercat_conf.status_filter != log_data.status))
	{
		return;
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

	fprintf(udploggercat_conf.log_destination, "%s%c[%s:%hu]%c%s\n", current_time_str, udploggercat_conf.delimiter_character, inet_ntoa(sender->sin_addr), ntohs(sender->sin_port), udploggercat_conf.delimiter_character, line);
}


void usage_hook()
{
	printf("  -d, --delimiter <delim>           set the delimiter to be used in-between log fields\n");
	printf("                                    (defaults to character 0x%x)\n", DELIMITER_CHARACTER);
	printf("  -f, --file <file>                 send log data to the file <file> (use `-' for stdout, which is the default)\n");
	printf("  -u, --request_url_filter <pcre>   only display log lines whose request_url field matches <pcre>\n");
	printf("  -s, --status_filter <status>      only display log lines whose status field matches <status>\n");
}
