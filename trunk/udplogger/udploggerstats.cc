#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <map>
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
	typedef std::map<short unsigned int, long unsigned int> map_t;
	static map_t status_maps[24]; /* 24 maps, one for each hour of the day (0..23). */

	if (data == NULL)
	{
		#ifdef __DEBUG__
			for (unsigned short i = 0; i < 24; i++)
			{
				for (map_t::const_iterator j = status_maps[i].begin(); j != status_maps[i].end(); j++)
				{
					printf("udploggerstats.c debug: status_maps[%hu].%u => %lu\n", i, j->first, j->second);
				}
			}
		#endif
		return;
	}
	else
	{
		status_maps[data->timestamp.tm_hour][data->status]++;
	}
}


void usersex_statistics(struct log_entry_t *data)
{
	typedef std::map<char *, long unsigned int> map_t;
	static map_t usersex_maps[24]; /* 24 maps, one for each hour of the day (0..23). */

	if (data == NULL)
	{
		#ifdef __DEBUG__
			for (unsigned short i = 0; i < 24; i++)
			{
				for (map_t::const_iterator j = usersex_maps[i].begin(); j != usersex_maps[i].end(); j++)
				{
					printf("udploggerstats.c debug: usersex_maps[%hu].%s => %lu\n", i, j->first, j->second);
				}
			}
		#endif
		return;
	}
	switch (data->nexopia_usersex)
	{
		case sex_male:
			usersex_maps[data->timestamp.tm_hour]["male"]++;
			break;
		case sex_female:
			usersex_maps[data->timestamp.tm_hour]["female"]++;
			break;
		default:
			usersex_maps[data->timestamp.tm_hour]["unknown"]++;
			break;
	}
}


void usertype_statistics(struct log_entry_t *data)
{
	typedef std::map<char *, long unsigned int> map_t;
	static map_t usertype_maps[24]; /* 24 maps, one for each hour of the day (0..23). */

	if (data == NULL)
	{
		#ifdef __DEBUG__
			for (unsigned short i = 0; i < 24; i++)
			{
				for (map_t::const_iterator j = usertype_maps[i].begin(); j != usertype_maps[i].end(); j++)
				{
					printf("udploggerstats.c debug: usertype_maps[%hu].%s => %lu\n", i, j->first, j->second);
				}
			}
		#endif
		return;
	}
	switch (data->nexopia_usertype)
	{
		case usertype_plus:
			usertype_maps[data->timestamp.tm_hour]["plus"]++;
			break;
		case usertype_user:
			usertype_maps[data->timestamp.tm_hour]["user"]++;
			break;
		case usertype_anon:
			usertype_maps[data->timestamp.tm_hour]["anon"]++;
			break;
		default:
			usertype_maps[data->timestamp.tm_hour]["unknown"]++;
			break;
	}
}
