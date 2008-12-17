#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sqlite3.h>
#include <map>
#include "udplogger.h"
#include "udploggerparselib.h"


/*
 * Structure that contains configuration information for the running instance
 * of this udploggerstats program.
 */
struct udploggerstats_configuration_t {
	sqlite3 *data_store;
} udploggerstats_conf;


int arguments_parse(int, char **);
void hit_statistics(struct log_entry_t *, time_t);
void status_statistics(struct log_entry_t *, time_t);
void usersex_statistics(struct log_entry_t *, time_t);
void usertype_statistics(struct log_entry_t *, time_t);


int main (int argc, char **argv)
{
	char *line = NULL;
	size_t line_buffer_length = 0;
	ssize_t line_length = 0;
	struct log_entry_t log_data;
	int result = 0;
	time_t timestamp = 0;

	result = arguments_parse(argc, argv);
	if (result <= 0)
	{
		return result;
	}

	while ((line_length = getline(&line, &line_buffer_length, stdin)) != -1)
	{
		parse_log_line(line, &log_data);

		/*
		 * Zero the tm_sec and tm_min fields of log_data.timestamp.  We only
		 * want granularity down to the hour.  Assign the timestamp local variable
		 * to be equivalent to the log_data.timestamp struct, except for stored as
		 * a time_t rather than a struct tm.
		 */
		log_data.timestamp.tm_sec = 0;
		log_data.timestamp.tm_min = 0;
		timestamp = mktime(&(log_data.timestamp));

		hit_statistics(&log_data, timestamp);
		status_statistics(&log_data, timestamp);
		usersex_statistics(&log_data, timestamp);
		usertype_statistics(&log_data, timestamp);
	}

	hit_statistics(NULL, 0);
	status_statistics(NULL, 0);
	usersex_statistics(NULL, 0);
	usertype_statistics(NULL, 0);

	sqlite3_close(udploggerstats_conf.data_store);
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
		{"database", required_argument, 0, 'd'},
		{"help", no_argument, 0, 'h'},
		{"version", no_argument, 0, 'v'},
		{0, 0, 0, 0}
	};
	int result;


	/* Initialize our configuration to the default settings. */
	udploggerstats_conf.data_store = NULL;

	while (1)
	{
		i = getopt_long(argc, argv, "d:hv", long_options, NULL);
		if (i == -1)
		{
			break;
		}
		switch (i)
		{
			case 'd':
				result = sqlite3_open(optarg, &(udploggerstats_conf.data_store));
				if (result != SQLITE_OK)
				{
					printf("udploggerstats.cc unable to open sqlite database '%s': %s\n", optarg, sqlite3_errmsg(udploggerstats_conf.data_store));
					udploggerstats_conf.data_store = NULL;
					return -1;
				}
				else
				{
					#ifdef __DEBUG__
						printf("udploggerstats.cc debug: opened sqlite database '%s' as data store\n", optarg);
					#endif
				}
				break;
			case 'h':
				printf("Usage: udploggerstats [OPTIONS]\n");
				printf("\n");
				printf("  -d, --database <db path>                       use <db path> as the sqlite data store for statistic storage\n");
				printf("  -h, --help                                     display this help and exit\n");
				printf("  -v, --version                                  display udploggerstats version and exit\n");
				printf("\n");
				return 0;
			case 'v':
				printf("udploggerstats.cc revision r%d\n", REVISION);
				return 0;
		}
	}

	return 1;
}


void hit_statistics(struct log_entry_t *data, time_t timestamp)
{
	typedef std::map<char *, long unsigned int> hit_map_t;
	typedef std::map<time_t, hit_map_t> timestamp_hit_map_t;
	static timestamp_hit_map_t hit_maps;
	char sql[200];
	char *sqlite_errmsg = NULL;

	if (data != NULL)
	{
		hit_maps[timestamp]["hits"]++;
		hit_maps[timestamp]["bytes_incoming"] += data->bytes_incoming;
		hit_maps[timestamp]["bytes_outgoing"] += data->bytes_outgoing;
		return;
	}

	if (udploggerstats_conf.data_store != NULL)
	{
		if (sqlite3_exec(udploggerstats_conf.data_store, "CREATE TABLE IF NOT EXISTS hit_statistics ( timestamp INTEGER, hits INTEGER, bytes_incoming INTEGER, bytes_outgoing INTEGER, PRIMARY KEY (timestamp) );", NULL, NULL, &sqlite_errmsg) != SQLITE_OK)
		{
			fprintf(stderr, "udploggerstats.cc could not ensure existence of table hit_statistics: %s\n", sqlite_errmsg);
		}
		if (sqlite_errmsg != NULL)
		{
			sqlite3_free(sqlite_errmsg);
			sqlite_errmsg = NULL;
		}
	}

	for (timestamp_hit_map_t::const_iterator i = hit_maps.begin(); i != hit_maps.end(); i++)
	{
		timestamp = i->first;
		hit_map_t hit_map = i->second;

		if (udploggerstats_conf.data_store != NULL)
		{
			snprintf(sql, 200, "INSERT OR IGNORE INTO hit_statistics ( timestamp, hits, bytes_incoming, bytes_outgoing ) VALUES ( %ld, 0, 0, 0 );", timestamp);
			if (sqlite3_exec(udploggerstats_conf.data_store, sql, NULL, NULL, &sqlite_errmsg) != SQLITE_OK)
			{
				fprintf(stderr, "udploggerstats.cc could not execute insertion into hit_statistics: %s\n", sqlite_errmsg);
			}
			if (sqlite_errmsg != NULL)
			{
				sqlite3_free(sqlite_errmsg);
				sqlite_errmsg = NULL;
			}

			snprintf(sql, 200, "UPDATE hit_statistics SET hits = hits + %lu, bytes_incoming = bytes_incoming + %lu, bytes_outgoing = bytes_outgoing + %lu WHERE timestamp = %ld;", hit_map["hits"], hit_map["bytes_incoming"], hit_map["bytes_outgoing"], timestamp);
			if (sqlite3_exec(udploggerstats_conf.data_store, sql, NULL, NULL, &sqlite_errmsg) != SQLITE_OK)
			{
				fprintf(stderr, "udploggerstats.cc could not execute update of hit_statistics: %s\n", sqlite_errmsg);
			}
			if (sqlite_errmsg != NULL)
			{
				sqlite3_free(sqlite_errmsg);
				sqlite_errmsg = NULL;
			}
		}

		#ifdef __DEBUG__
			for (hit_map_t::const_iterator j = hit_map.begin(); j != hit_map.end(); j++)
			{
				printf("udploggerstats.cc debug: hit_maps[%ld].%s => %lu\n", timestamp, j->first, j->second);
			}
		#endif
	}
}


void status_statistics(struct log_entry_t *data, time_t timestamp)
{
	typedef std::map<short unsigned int, long unsigned int> status_map_t;
	typedef std::map<time_t, status_map_t> timestamp_status_map_t;
	static timestamp_status_map_t status_maps;
	char sql[200];
	char *sqlite_errmsg = NULL;

	if (data != NULL)
	{
		status_maps[timestamp][data->status]++;
		return;
	}

	if (udploggerstats_conf.data_store != NULL)
	{
		if (sqlite3_exec(udploggerstats_conf.data_store, "CREATE TABLE IF NOT EXISTS status_statistics ( timestamp INTEGER, status INTEGER, count INTEGER, PRIMARY KEY (timestamp, status) );", NULL, NULL, &sqlite_errmsg) != SQLITE_OK)
		{
			fprintf(stderr, "udploggerstats.cc could not ensure existence of table status_statistics: %s\n", sqlite_errmsg);
		}
		if (sqlite_errmsg != NULL)
		{
			sqlite3_free(sqlite_errmsg);
			sqlite_errmsg = NULL;
		}
	}

	for (timestamp_status_map_t::const_iterator i = status_maps.begin(); i != status_maps.end(); i++)
	{
		timestamp = i->first;
		status_map_t status_map = i->second;

		for (status_map_t::const_iterator j = status_map.begin(); j != status_map.end(); j++)
		{
			short unsigned int status = j->first;
			long unsigned int count = j->second;

			if (udploggerstats_conf.data_store != NULL)
			{
				snprintf(sql, 200, "INSERT OR IGNORE INTO status_statistics ( timestamp, status, count ) VALUES ( %ld, %hu, 0 );", timestamp, status);
				if (sqlite3_exec(udploggerstats_conf.data_store, sql, NULL, NULL, &sqlite_errmsg) != SQLITE_OK)
				{
					fprintf(stderr, "udploggerstats.cc could not execute insertion into status_statistics: %s\n", sqlite_errmsg);
				}
				if (sqlite_errmsg != NULL)
				{
					sqlite3_free(sqlite_errmsg);
					sqlite_errmsg = NULL;
				}

				snprintf(sql, 200, "UPDATE status_statistics SET count = count + %lu WHERE timestamp = %ld AND status = %hu;", count, timestamp, status);
				if (sqlite3_exec(udploggerstats_conf.data_store, sql, NULL, NULL, &sqlite_errmsg) != SQLITE_OK)
				{
					fprintf(stderr, "udploggerstats.cc could not execute update of status_statistics: %s\n", sqlite_errmsg);
				}
				if (sqlite_errmsg != NULL)
				{
					sqlite3_free(sqlite_errmsg);
					sqlite_errmsg = NULL;
				}
			}

			#ifdef __DEBUG__
				printf("udploggerstats.cc debug: status_maps[%ld].%hu => %lu\n", timestamp, status, count);
			#endif
		}
	}
}


void usersex_statistics(struct log_entry_t *data, time_t timestamp)
{
	typedef std::map<char *, long unsigned int> usersex_map_t;
	typedef std::map<time_t, usersex_map_t> timestamp_usersex_map_t;
	static timestamp_usersex_map_t usersex_maps;
	char sql[200];
	char *sqlite_errmsg = NULL;

	if (data != NULL)
	{
		switch (data->nexopia_usersex)
		{
			case sex_male:
				usersex_maps[timestamp]["male"]++;
				break;
			case sex_female:
				usersex_maps[timestamp]["female"]++;
				break;
			default:
				usersex_maps[timestamp]["unknown"]++;
				break;
		}
		return;
	}

	if (udploggerstats_conf.data_store != NULL)
	{
		if (sqlite3_exec(udploggerstats_conf.data_store, "CREATE TABLE IF NOT EXISTS usersex_statistics ( timestamp INTEGER, sex TEXT, count INTEGER, PRIMARY KEY (timestamp, sex) );", NULL, NULL, &sqlite_errmsg) != SQLITE_OK)
		{
			fprintf(stderr, "udploggerstats.cc could not ensure existence of table usersex_statistics: %s\n", sqlite_errmsg);
		}
		if (sqlite_errmsg != NULL)
		{
			sqlite3_free(sqlite_errmsg);
			sqlite_errmsg = NULL;
		}
	}

	for (timestamp_usersex_map_t::const_iterator i = usersex_maps.begin(); i != usersex_maps.end(); i++)
	{
		timestamp = i->first;
		usersex_map_t usersex_map = i->second;

		for (usersex_map_t::const_iterator j = usersex_map.begin(); j != usersex_map.end(); j++)
		{
			char *sex = j->first;
			long unsigned int count = j->second;

			if (udploggerstats_conf.data_store != NULL)
			{
				snprintf(sql, 200, "INSERT OR IGNORE INTO usersex_statistics ( timestamp, sex, count ) VALUES ( %ld, '%s', 0 );", timestamp, sex);
				if (sqlite3_exec(udploggerstats_conf.data_store, sql, NULL, NULL, &sqlite_errmsg) != SQLITE_OK)
				{
					fprintf(stderr, "udploggerstats.cc could not execute insertion into usersex_statistics: %s\n", sqlite_errmsg);
				}
				if (sqlite_errmsg != NULL)
				{
					sqlite3_free(sqlite_errmsg);
					sqlite_errmsg = NULL;
				}

				snprintf(sql, 200, "UPDATE usersex_statistics SET count = count + %lu WHERE timestamp = %ld AND sex = '%s';", count, timestamp, sex);
				if (sqlite3_exec(udploggerstats_conf.data_store, sql, NULL, NULL, &sqlite_errmsg) != SQLITE_OK)
				{
					fprintf(stderr, "udploggerstats.cc could not execute update of usersex_statistics: %s\n", sqlite_errmsg);
				}
				if (sqlite_errmsg != NULL)
				{
					sqlite3_free(sqlite_errmsg);
					sqlite_errmsg = NULL;
				}
			}

			#ifdef __DEBUG__
				printf("udploggerstats.cc debug: usersex_maps[%ld].%s => %lu\n", timestamp, sex, count);
			#endif
		}
	}
}


void usertype_statistics(struct log_entry_t *data, time_t timestamp)
{
	typedef std::map<char *, long unsigned int> usertype_map_t;
	typedef std::map<time_t, usertype_map_t> timestamp_usertype_map_t;
	static timestamp_usertype_map_t usertype_maps;
	char sql[200];
	char *sqlite_errmsg = NULL;

	if (data != NULL)
	{
		switch (data->nexopia_usertype)
		{
			case usertype_plus:
				usertype_maps[timestamp]["plus"]++;
				break;
			case usertype_user:
				usertype_maps[timestamp]["user"]++;
				break;
			case usertype_anon:
				usertype_maps[timestamp]["anon"]++;
				break;
			default:
				usertype_maps[timestamp]["unknown"]++;
				break;
		}
		return;
	}

	if (udploggerstats_conf.data_store != NULL)
	{
		if (sqlite3_exec(udploggerstats_conf.data_store, "CREATE TABLE IF NOT EXISTS usertype_statistics ( timestamp INTEGER, type TEXT, count INTEGER, PRIMARY KEY (timestamp, type) );", NULL, NULL, &sqlite_errmsg) != SQLITE_OK)
		{
			fprintf(stderr, "udploggerstats.cc could not ensure existence of table usertype_statistics: %s\n", sqlite_errmsg);
		}
		if (sqlite_errmsg != NULL)
		{
			sqlite3_free(sqlite_errmsg);
			sqlite_errmsg = NULL;
		}
	}

	for (timestamp_usertype_map_t::const_iterator i = usertype_maps.begin(); i != usertype_maps.end(); i++)
	{
		timestamp = i->first;
		usertype_map_t usertype_map = i->second;

		for (usertype_map_t::const_iterator j = usertype_map.begin(); j != usertype_map.end(); j++)
		{
			char *type = j->first;
			long unsigned int count = j->second;

			if (udploggerstats_conf.data_store != NULL)
			{
				snprintf(sql, 200, "INSERT OR IGNORE INTO usertype_statistics ( timestamp, type, count ) VALUES ( %ld, '%s', 0 );", timestamp, type);
				if (sqlite3_exec(udploggerstats_conf.data_store, sql, NULL, NULL, &sqlite_errmsg) != SQLITE_OK)
				{
					fprintf(stderr, "udploggerstats.cc could not execute insertion into usertype_statistics: %s\n", sqlite_errmsg);
				}
				if (sqlite_errmsg != NULL)
				{
					sqlite3_free(sqlite_errmsg);
					sqlite_errmsg = NULL;
				}

				snprintf(sql, 200, "UPDATE usertype_statistics SET count = count + %lu WHERE timestamp = %ld AND type = '%s';", count, timestamp, type);
				if (sqlite3_exec(udploggerstats_conf.data_store, sql, NULL, NULL, &sqlite_errmsg) != SQLITE_OK)
				{
					fprintf(stderr, "udploggerstats.cc could not execute update of usertype_statistics: %s\n", sqlite_errmsg);
				}
				if (sqlite_errmsg != NULL)
				{
					sqlite3_free(sqlite_errmsg);
					sqlite_errmsg = NULL;
				}
			}

			#ifdef __DEBUG__
				printf("udploggerstats.cc debug: usertype_maps[%ld].%s => %lu\n", timestamp, type, count);
			#endif
		}
	}
}
