#include <arpa/inet.h>
#include <getopt.h>
#include <inttypes.h>
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
 * The size of the memory to allocate for working with time string buffers.  This
 * is used to store the current timestamp in a string for easy printing and also
 * defines the maximum length of udploggerc_conf.log_destination_path, which is based
 * on udploggerc_conf.log_destination_format.
 */
#define TIME_STRING_BUFFER_SIZE 513U


/*
 * Structure that contains configuration information for the running instance
 * of this udploggerc client program.
 */
struct udploggerc_configuration_t {
	unsigned char delimiter_character;
	FILE *log_destination;
	char *log_destination_format;
	char log_destination_path[TIME_STRING_BUFFER_SIZE];
} udploggerc_conf;


int add_option_hook();
static void close_log_file();
int getopt_hook(char);
void handle_signal_hook(sigset_t *);
void inline log_packet_hook(struct sockaddr_in *, char *);
static int open_log_file(char *);
void usage_hook();


int add_option_hook()
{
	udploggerc_conf.delimiter_character = DELIMITER_CHARACTER;
	udploggerc_conf.log_destination = stdout;
	udploggerc_conf.log_destination_format = NULL;
	memset(udploggerc_conf.log_destination_path, '\0', TIME_STRING_BUFFER_SIZE * sizeof(char));
	return
	(
		add_option("delimiter", required_argument, 'd') &&
		add_option("file", required_argument, 'f')
	);
}


static void close_log_file()
{
	if (strnlen(udploggerc_conf.log_destination_path, TIME_STRING_BUFFER_SIZE) > 0)
	{
		#ifdef __DEBUG__
			printf("udploggerc.c debug: closing log destination '%s'\n", udploggerc_conf.log_destination_path);
		#endif
		memset(udploggerc_conf.log_destination_path, '\0', TIME_STRING_BUFFER_SIZE * sizeof(char));
	}
	if (udploggerc_conf.log_destination != stdout)
	{
		if (fclose(udploggerc_conf.log_destination))
		{
			perror("udploggerc.c fclose()");
		}
		udploggerc_conf.log_destination = stdout;
	}
}


int getopt_hook(char i)
{
	switch (i)
	{
		case 'd':
			if (strlen(optarg) != 1)
			{
				fprintf(stderr, "udploggerc.c invalid delimiter '%s'\n", optarg);
				return -1;
			}
			else
			{
				udploggerc_conf.delimiter_character = optarg[0];
				#ifdef __DEBUG__
					printf("udploggerc.c debug: setting delimiter to '%c' (0x%x)\n", udploggerc_conf.delimiter_character, udploggerc_conf.delimiter_character);
				#endif
				return 1;
			}
		case 'f':
			if (strcmp("-", optarg))
			{
				if (strlen(optarg) >= TIME_STRING_BUFFER_SIZE)
				{
					fprintf(stderr, "udploggerc.c log file format specification is too long, maximum length is %du characters\n", TIME_STRING_BUFFER_SIZE - 1);
					return -1;
				}
				else
				{
					udploggerc_conf.log_destination_format = strdup(optarg);
					if (udploggerc_conf.log_destination_format == NULL)
					{
						perror("udploggerc.c strdup()");
						return -1;
					}
					#ifdef __DEBUG__
						printf("udploggerc.c debug: setting log file format specification to '%s'\n", udploggerc_conf.log_destination_format);
					#endif
				}
			}
			else
			{
				#ifdef __DEBUG__
					printf("udploggerc.c debug: setting output file to stdout\n");
				#endif
			}
			return 1;
	}
	return 0;
}


void handle_signal_hook(sigset_t *signal_flags)
{

	if (sigismember(signal_flags, SIGHUP))
	{
		#ifdef __DEBUG__
			printf("udploggerc.c debug: HUP received\n");
		#endif
		if (strnlen(udploggerc_conf.log_destination_path, TIME_STRING_BUFFER_SIZE) > 0)
		{
			open_log_file(udploggerc_conf.log_destination_path);
		}
	}
	if (sigismember(signal_flags, SIGTERM))
	{
		#ifdef __DEBUG__
			printf("udploggerc.c debug: TERM received\n");
		#endif
		close_log_file();
	}
}


void inline log_packet_hook(struct sockaddr_in *sender, char *line)
{
	static time_t current_timestamp = 0;
	static struct tm current_time;
	static char current_time_str[TIME_STRING_BUFFER_SIZE];
	int i;
	static struct log_entry_t log_data;
	static char new_log_destination_path[TIME_STRING_BUFFER_SIZE];

	/*
	 * Parse the log line into the structure log_data.  This is only used if filters are in-place, but we do it all the time
         * for the sake of simplicity.  If we run into speed issues this can be optimized out to run only when required.
	 */
	bzero(&log_data, sizeof(log_data));
	parse_log_line(line, &log_data);

	/* Update our timestamp string (if necessary). */
	if ((! current_timestamp) || (current_timestamp != time(NULL)))
	{
		current_timestamp = time(NULL);
		if (current_timestamp == (time_t)-1)
		{
			perror("udploggerc.c time()");
			return;
		}
		if (localtime_r(&current_timestamp, &current_time) == NULL)
		{
			perror("udploggerc.c localtime_r()");
			return;
		}
		strftime(current_time_str, TIME_STRING_BUFFER_SIZE, "[%Y-%m-%d %H:%M:%S]", &current_time);
		current_time_str[TIME_STRING_BUFFER_SIZE - 1] = '\0';

		/* Update our log file destination path (if necessary). */
		if (udploggerc_conf.log_destination_format != NULL)
		{
			strftime(new_log_destination_path, TIME_STRING_BUFFER_SIZE, udploggerc_conf.log_destination_format, &current_time);
			new_log_destination_path[TIME_STRING_BUFFER_SIZE - 1] = '\0';
			if (strncmp(new_log_destination_path, udploggerc_conf.log_destination_path, TIME_STRING_BUFFER_SIZE))
			{
				open_log_file(new_log_destination_path);
			}
		}
	}

	/* Format the log line for output. */
	if (udploggerc_conf.delimiter_character != DELIMITER_CHARACTER)
	{
		for (i = 0; i < strnlen(line, PACKET_MAXIMUM_SIZE); i++)
		{
			if (line[i] == DELIMITER_CHARACTER)
			{
				line[i] = udploggerc_conf.delimiter_character;
			}
		}
	}

	fprintf(udploggerc_conf.log_destination, "%s%c[%s:%hu]%c%s\n", current_time_str, udploggerc_conf.delimiter_character, inet_ntoa(sender->sin_addr), ntohs(sender->sin_port), udploggerc_conf.delimiter_character, line);
}


static int open_log_file(char *log_file_path)
{
	FILE *new_log_destination = NULL;

	#ifdef __DEBUG__
		printf("udploggerc.c debug: changing log destination to '%s'\n", log_file_path);
	#endif
	new_log_destination = fopen(log_file_path, "a");
	if (new_log_destination == NULL)
	{
		perror("udploggerc.c fopen()");
		fprintf(stderr, "udploggerc.c could not open file '%s' for appending\n", log_file_path);
		return 0;
	}
	close_log_file();
	udploggerc_conf.log_destination = new_log_destination;
	strncpy(udploggerc_conf.log_destination_path, log_file_path, TIME_STRING_BUFFER_SIZE);
	udploggerc_conf.log_destination_path[TIME_STRING_BUFFER_SIZE - 1] = '\0';
	return 1;
}


void usage_hook()
{
	printf("  -d, --delimiter <delim>           set the delimiter to be used in-between log fields\n");
	printf("                                    (defaults to character 0x%x)\n", DELIMITER_CHARACTER);
	printf("  -f, --file <file>                 send log data to the file <file> (use `-' for stdout, which is the default)\n");
	printf("                                    <file> can be a format specification and supports conversion specifications as per strftime(3)\n");
}
