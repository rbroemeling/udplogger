#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <getopt.h>
#include <pcre.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include "udplogger.h"
#include "udploggerparselib.h"


/*
 * Number of elements to use in the ovector for pcre result substrings.
 * Should be a multiple of 3.
 */
#define OVECCOUNT 3


/*
 * Structure that contains configuration information for the running instance
 * of this udploggergrep program.
 */
struct udploggergrep_configuration_t {
	pcre *query_filter;
	uint16_t status_filter;
	pcre *url_filter;
} udploggergrep_conf;


int arguments_parse(int, char **);


int main (int argc, char **argv)
{
	char *line = NULL;
	size_t line_buffer_length = 0;
	ssize_t line_length = 0;
	struct log_entry_t log_data;
	static int ovector[OVECCOUNT];
	int result = 0;

	result = arguments_parse(argc, argv);
	if (result <= 0)
	{
		return result;
	}

	while ((line_length = getline(&line, &line_buffer_length, stdin)) != -1)
	{
		parse_log_line(line, &log_data);

		/* Filter out anything that we do not want to display. */
		if (udploggergrep_conf.status_filter && (udploggergrep_conf.status_filter != log_data.status))
		{
			continue;
		}
		if (udploggergrep_conf.query_filter != NULL)
		{
			result = pcre_exec(udploggergrep_conf.query_filter, NULL, log_data.query_string, strlen(log_data.query_string), 0, 0, ovector, OVECCOUNT);
			if (result < 0)
			{
				continue;
			}
		}
		if (udploggergrep_conf.url_filter != NULL)
		{
			result = pcre_exec(udploggergrep_conf.url_filter, NULL, log_data.request_url, strlen(log_data.request_url), 0, 0, ovector, OVECCOUNT);
			if (result < 0)
			{
				continue;
			}
		}

		printf("%s", line);
	}
	return 0;
}


/**
 * arguments_parse(argc, argv)
 *
 * Utility function to parse the passed in arguments using getopt; also responsible for sanity-checking
 * any passed-in values and ensuring that the program's configuration is sane before returning (i.e.
 * using default values).
 **/
int arguments_parse(int argc, char **argv)
{
	int i;
	static struct option long_options[] =
	{
		{"help", no_argument, 0, 'h'},
		{"query", required_argument, 0, 'q'},
		{"status", required_argument, 0, 's'},
		{"url", required_argument, 0, 'u'},
		{"version", no_argument, 0, 'v'},
		{0, 0, 0, 0}
	};
	const char *pcre_error;
	int pcre_error_offset;
	uintmax_t uint_tmp;


	/* Initialize our configuration to the default settings. */
	udploggergrep_conf.query_filter = NULL;
	udploggergrep_conf.status_filter = 0;
	udploggergrep_conf.url_filter = NULL;

	while (1)
	{
		i = getopt_long(argc, argv, "hq:s:u:v", long_options, NULL);
		if (i == -1)
		{
			break;
		}
		switch (i)
		{
			case 'h':
				printf("Usage: udploggergrep [OPTIONS]\n");
				printf("\n");
				printf("  -h, --help                                     display this help and exit\n");
				printf("  -q, --query <regexp>                           show log entries whose query string matches <regexp>\n");
				printf("  -s, --status <status code>                     show log entries whose status code equals <status code>\n");
				printf("  -u, --url <regexp>                             show log entries whose url matches <regexp>\n");
				printf("  -v, --version                                  display udploggergrep version and exit\n");
				printf("\n");
				return 0;
			case 'q':
				udploggergrep_conf.query_filter = pcre_compile(optarg, 0, &pcre_error, &pcre_error_offset, NULL);
				if (udploggergrep_conf.query_filter == NULL)
				{
					fprintf(stderr, "udploggergrep.cc invalid query regexp at offset %d: %s\n", pcre_error_offset, pcre_error);
					return -1;
				}
				else
				{
					#ifdef __DEBUG__
						printf("udploggergrep.cc debug: setting query regexp to '%s'\n", optarg);
					#endif
				}
				break;
			case 's':
				uint_tmp = strtoumax(optarg, 0, 10);
				if (! uint_tmp || uint_tmp == UINT_MAX || uint_tmp > 65535)
				{
					fprintf(stderr, "udploggergrep.cc invalid status filter '%s'\n", optarg);
					return -1;
				}
				else
				{
					udploggergrep_conf.status_filter = uint_tmp;
					#ifdef __DEBUG__
						printf("udploggergrep.cc debug: setting status_filter to '%hu'\n", udploggergrep_conf.status_filter);
					#endif
				}
				break;
			case 'u':
				udploggergrep_conf.url_filter = pcre_compile(optarg, 0, &pcre_error, &pcre_error_offset, NULL);
				if (udploggergrep_conf.url_filter == NULL)
				{
					fprintf(stderr, "udploggergrep.cc invalid url regexp at offset %d: %s\n", pcre_error_offset, pcre_error);
					return -1;
				}
				else
				{
					#ifdef __DEBUG__
						printf("udploggergrep.cc debug: setting url regexp to '%s'\n", optarg);
					#endif
				}
				break;
			case 'v':
				printf("udploggergrep.cc revision r%d\n", REVISION);
				return 0;
		}
	}

	return 1;
}
