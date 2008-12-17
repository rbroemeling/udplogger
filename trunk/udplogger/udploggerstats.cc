#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include "udplogger.h"
#include "udploggerparselib.h"


/*
 * Structure that contains configuration information for the running instance
 * of this udploggerstats program.
 */
struct udploggerstats_configuration_t {
	int conf;
} udploggerstats_conf;


void status_statistics(struct log_entry_t *);
void usersex_statistics(struct log_entry_t *);
void usertype_statistics(struct log_entry_t *);


int main (int argc, char **argv)
{
	char *line = NULL;
	size_t line_buffer_length = 0;
	ssize_t line_length = 0;
	struct log_entry_t log_data;

	while ((line_length = getline(&line, &line_buffer_length, stdin)) != -1)
	{
		parse_log_line(line, &log_data);
		status_statistics(&log_data);
		usersex_statistics(&log_data);
		usertype_statistics(&log_data);
	}

	status_statistics(NULL);
	usersex_statistics(NULL);
	usertype_statistics(NULL);
	return 0;
}


void status_statistics(struct log_entry_t *data)
{
	unsigned int i;
	static short unsigned int initialization_flag = 1;
	static long unsigned int status_count[USHRT_MAX + 1];

	if (initialization_flag)
	{
		bzero(&status_count, (USHRT_MAX + 1) * sizeof(long unsigned int));
		initialization_flag = 0;
	}
	if (data == NULL)
	{
		#ifdef __DEBUG__
			for (i = 0; i < (USHRT_MAX + 1); i++)
			{
				if (status_count[i] > 0)
				{
					printf("udploggerstats.c debug: status_statistics.%u => %lu\n", i, status_count[i]);
				}
			}
		#endif
		return;
	}
	else
	{
		status_count[data->status]++;
	}
}


void usersex_statistics(struct log_entry_t *data)
{
	static long unsigned int unknown = 0;
	static long unsigned int female = 0;
	static long unsigned int male = 0;

	if (data == NULL)
	{
		#ifdef __DEBUG__
			printf("udploggerstats.c debug: usersex_statistics.male => %lu\n", male);
			printf("udploggerstats.c debug: usersex_statistics.female => %lu\n", female);
			printf("udploggerstats.c debug: usersex_statistics.unknown => %lu\n", unknown);
		#endif
		return;
	}
	switch (data->nexopia_usersex)
	{
		case sex_male:
			male++;
			break;
		case sex_female:
			female++;
			break;
		default:
			unknown++;
			break;
	}
}


void usertype_statistics(struct log_entry_t *data)
{
	static long unsigned int unknown = 0;
	static long unsigned int plus = 0;
	static long unsigned int user = 0;
	static long unsigned int anon = 0;

	if (data == NULL)
	{
		#ifdef __DEBUG__
			printf("udploggerstats.c debug: usertype_statistics.plus => %lu\n", plus);
			printf("udploggerstats.c debug: usertype_statistics.user => %lu\n", user);
			printf("udploggerstats.c debug: usertype_statistics.anon => %lu\n", anon);
			printf("udploggerstats.c debug: usertype_statistics.unknown => %lu\n", unknown);
		#endif
		return;
	}
	switch (data->nexopia_usertype)
	{
		case usertype_plus:
			plus++;
			break;
		case usertype_user:
			user++;
			break;
		case usertype_anon:
			anon++;
			break;
		default:
			unknown++;
			break;
	}
}
