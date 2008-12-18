#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <getopt.h>
#include <map>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <string.h>
#include <strings.h>
#include <sqlite3.h>
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
void content_type_statistics(struct log_entry_t *, time_t);
void hit_statistics(struct log_entry_t *, time_t);
void host_statistics(struct log_entry_t *, time_t);
void sql_query(const char *, ...);
void status_statistics(struct log_entry_t *, time_t);
void time_used_statistics(struct log_entry_t *, time_t);
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

		content_type_statistics(&log_data, timestamp);
		hit_statistics(&log_data, timestamp);
		host_statistics(&log_data, timestamp);
		status_statistics(&log_data, timestamp);
		time_used_statistics(&log_data, timestamp);
		usersex_statistics(&log_data, timestamp);
		usertype_statistics(&log_data, timestamp);
	}

	content_type_statistics(NULL, 0);
	hit_statistics(NULL, 0);
	host_statistics(NULL, 0);
	status_statistics(NULL, 0);
	time_used_statistics(NULL, 0);
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


void content_type_statistics(struct log_entry_t *data, time_t timestamp)
{
	typedef std::map<std::string, long unsigned int> content_type_map_t;
	typedef std::map<time_t, content_type_map_t> timestamp_content_type_map_t;
	static timestamp_content_type_map_t content_type_maps;

	if (data != NULL)
	{
		std::string content_type(data->content_type);
		content_type_maps[timestamp][content_type]++;
		return;
	}

	sql_query("CREATE TABLE IF NOT EXISTS content_type_statistics ( timestamp INTEGER, content_type TEXT, count INTEGER, PRIMARY KEY (timestamp, content_type) );");

	for (timestamp_content_type_map_t::const_iterator i = content_type_maps.begin(); i != content_type_maps.end(); i++)
	{
		timestamp = i->first;
		content_type_map_t content_type_map = i->second;

		for (content_type_map_t::const_iterator j = content_type_map.begin(); j != content_type_map.end(); j++)
		{
			std::string content_type = j->first;
			long unsigned int count = j->second;

			sql_query("INSERT OR IGNORE INTO content_type_statistics ( timestamp, content_type, count ) VALUES ( %ld, '%s', 0 );", timestamp, content_type.c_str());
			sql_query("UPDATE content_type_statistics SET count = count + %lu WHERE timestamp = %ld AND host = '%s';", count, timestamp, content_type.c_str());

			#ifdef __DEBUG__
				printf("udploggerstats.cc debug: content_type_maps[%ld].%s => %lu\n", timestamp, content_type.c_str(), count);
			#endif
		}
	}
}


void hit_statistics(struct log_entry_t *data, time_t timestamp)
{
	typedef std::map<char *, long unsigned int> hit_map_t;
	typedef std::map<time_t, hit_map_t> timestamp_hit_map_t;
	static timestamp_hit_map_t hit_maps;

	if (data != NULL)
	{
		hit_maps[timestamp]["hits"]++;
		hit_maps[timestamp]["bytes_incoming"] += data->bytes_incoming;
		hit_maps[timestamp]["bytes_outgoing"] += data->bytes_outgoing;
		return;
	}

	sql_query("CREATE TABLE IF NOT EXISTS hit_statistics ( timestamp INTEGER, hits INTEGER, bytes_incoming INTEGER, bytes_outgoing INTEGER, PRIMARY KEY (timestamp) );");

	for (timestamp_hit_map_t::const_iterator i = hit_maps.begin(); i != hit_maps.end(); i++)
	{
		timestamp = i->first;
		hit_map_t hit_map = i->second;

		sql_query("INSERT OR IGNORE INTO hit_statistics ( timestamp, hits, bytes_incoming, bytes_outgoing ) VALUES ( %ld, 0, 0, 0 );", timestamp);
		sql_query("UPDATE hit_statistics SET hits = hits + %lu, bytes_incoming = bytes_incoming + %lu, bytes_outgoing = bytes_outgoing + %lu WHERE timestamp = %ld;", hit_map["hits"], hit_map["bytes_incoming"], hit_map["bytes_outgoing"], timestamp);

		#ifdef __DEBUG__
			for (hit_map_t::const_iterator j = hit_map.begin(); j != hit_map.end(); j++)
			{
				printf("udploggerstats.cc debug: hit_maps[%ld].%s => %lu\n", timestamp, j->first, j->second);
			}
		#endif
	}
}


void host_statistics(struct log_entry_t *data, time_t timestamp)
{
	typedef std::map<std::string, long unsigned int> host_map_t;
	typedef std::map<time_t, host_map_t> timestamp_host_map_t;
	static timestamp_host_map_t host_maps;

	if (data != NULL)
	{
		std::string host(data->host);
		host_maps[timestamp][host]++;
		return;
	}

	sql_query("CREATE TABLE IF NOT EXISTS host_statistics ( timestamp INTEGER, host TEXT, count INTEGER, PRIMARY KEY (timestamp, host) );");

	for (timestamp_host_map_t::const_iterator i = host_maps.begin(); i != host_maps.end(); i++)
	{
		timestamp = i->first;
		host_map_t host_map = i->second;

		for (host_map_t::const_iterator j = host_map.begin(); j != host_map.end(); j++)
		{
			std::string host = j->first;
			long unsigned int count = j->second;

			sql_query("INSERT OR IGNORE INTO host_statistics ( timestamp, host, count ) VALUES ( %ld, '%s', 0 );", timestamp, host.c_str());
			sql_query("UPDATE host_statistics SET count = count + %lu WHERE timestamp = %ld AND host = '%s';", count, timestamp, host.c_str());

			#ifdef __DEBUG__
				printf("udploggerstats.cc debug: host_maps[%ld].%s => %lu\n", timestamp, host.c_str(), count);
			#endif
		}
	}
}


void sql_query(const char *sql_fmt, ...)
{
	va_list ap;
	int result = 0;
	char *sql = NULL;
	int sql_size = 200; /* Guess that we will not need more than 200 bytes to properly render sql_fmt. */
	char *sqlite_errmsg = NULL;

	if (udploggerstats_conf.data_store == NULL)
	{
		return;
	}

	while (1)
	{
		if ((sql = (char *)calloc(sql_size, sizeof(char))) == NULL)
		{
			fprintf(stderr, "udploggerstats.cc could not allocate %d bytes for sql query string\n", sql_size);
			return;
		}

		/* Attempt to render sql_fmt in the memory pointed to by sql. */
		va_start(ap, sql_fmt);
		result = vsnprintf(sql, sql_size, sql_fmt, ap);
		va_end(ap);

		if ((result > -1) && (result < sql_size))
		{
			/* We rendered the string successfully. */
			break;
		}

		/* We failed to render the string, try again with more space. */
		if (result > -1)
		{
			sql_size = result + 1; /* glibc 2.1 - use exactly as much memory as we need. */
		}
		else
		{
			sql_size *= 2; /* glibc 2.0 - double the amount of memory that we attempt to use. */
		}
		free(sql);
		sql = NULL;
	}

	#ifdef __DEBUG__
		printf("udploggerstats.cc debug: SQL format '%s' was successfully rendered to '%s'.\n", sql_fmt, sql);
	#endif

	result = sqlite3_exec(udploggerstats_conf.data_store, sql, NULL, NULL, &sqlite_errmsg);
	if (result != SQLITE_OK)
	{
		fprintf(stderr, "udploggerstats.cc SQL query: %s\n", sql);
		fprintf(stderr, "udploggerstats.cc SQL error: %s\n", sqlite_errmsg);
	}
	if (sqlite_errmsg != NULL)
	{
		sqlite3_free(sqlite_errmsg);
		sqlite_errmsg = NULL;
	}
	free(sql);
}


void status_statistics(struct log_entry_t *data, time_t timestamp)
{
	typedef std::map<short unsigned int, long unsigned int> status_map_t;
	typedef std::map<time_t, status_map_t> timestamp_status_map_t;
	static timestamp_status_map_t status_maps;

	if (data != NULL)
	{
		status_maps[timestamp][data->status]++;
		return;
	}

	sql_query("CREATE TABLE IF NOT EXISTS status_statistics ( timestamp INTEGER, status INTEGER, count INTEGER, PRIMARY KEY (timestamp, status) );");

	for (timestamp_status_map_t::const_iterator i = status_maps.begin(); i != status_maps.end(); i++)
	{
		timestamp = i->first;
		status_map_t status_map = i->second;

		for (status_map_t::const_iterator j = status_map.begin(); j != status_map.end(); j++)
		{
			short unsigned int status = j->first;
			long unsigned int count = j->second;

			sql_query("INSERT OR IGNORE INTO status_statistics ( timestamp, status, count ) VALUES ( %ld, %hu, 0 );", timestamp, status);
			sql_query("UPDATE status_statistics SET count = count + %lu WHERE timestamp = %ld AND status = %hu;", count, timestamp, status);

			#ifdef __DEBUG__
				printf("udploggerstats.cc debug: status_maps[%ld].%hu => %lu\n", timestamp, status, count);
			#endif
		}
	}
}


void time_used_statistics(struct log_entry_t *data, time_t timestamp)
{
	typedef std::map<short int, long unsigned int> time_used_map_t;
	typedef std::map<time_t, time_used_map_t> timestamp_time_used_map_t;
	static timestamp_time_used_map_t time_used_maps;

	if (data != NULL)
	{
		if (data->time_used > 10)
		{
			/* -1 is used to represent time used of >= 11.  This is non-obvious,
			 * but then again no other value that I can think of _would_
			 * be obvious. */
			time_used_maps[timestamp][-1]++;
		}
		else
		{
			time_used_maps[timestamp][data->time_used]++;
		}
		return;
	}

	sql_query("CREATE TABLE IF NOT EXISTS time_used_statistics ( timestamp INTEGER, time_used INTEGER, count INTEGER, PRIMARY KEY (timestamp, time_used) );");

	for (timestamp_time_used_map_t::const_iterator i = time_used_maps.begin(); i != time_used_maps.end(); i++)
	{
		timestamp = i->first;
		time_used_map_t time_used_map = i->second;

		for (time_used_map_t::const_iterator j = time_used_map.begin(); j != time_used_map.end(); j++)
		{
			short int time_used = j->first;
			long unsigned int count = j->second;

			sql_query("INSERT OR IGNORE INTO time_used_statistics ( timestamp, time_used, count ) VALUES ( %ld, %hd, 0 );", timestamp, time_used);
			sql_query("UPDATE time_used_statistics SET count = count + %lu WHERE timestamp = %ld AND status = %hd;", count, timestamp, time_used);

			#ifdef __DEBUG__
				printf("udploggerstats.cc debug: time_used_maps[%ld].%hd => %lu\n", timestamp, time_used, count);
			#endif
		}
	}
}


void usersex_statistics(struct log_entry_t *data, time_t timestamp)
{
	typedef std::map<char *, long unsigned int> usersex_map_t;
	typedef std::map<time_t, usersex_map_t> timestamp_usersex_map_t;
	static timestamp_usersex_map_t usersex_maps;

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

	sql_query("CREATE TABLE IF NOT EXISTS usersex_statistics ( timestamp INTEGER, sex TEXT, count INTEGER, PRIMARY KEY (timestamp, sex) );");

	for (timestamp_usersex_map_t::const_iterator i = usersex_maps.begin(); i != usersex_maps.end(); i++)
	{
		timestamp = i->first;
		usersex_map_t usersex_map = i->second;

		for (usersex_map_t::const_iterator j = usersex_map.begin(); j != usersex_map.end(); j++)
		{
			char *sex = j->first;
			long unsigned int count = j->second;

			sql_query("INSERT OR IGNORE INTO usersex_statistics ( timestamp, sex, count ) VALUES ( %ld, '%s', 0 );", timestamp, sex);
			sql_query("UPDATE usersex_statistics SET count = count + %lu WHERE timestamp = %ld AND sex = '%s';", count, timestamp, sex);

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

	sql_query("CREATE TABLE IF NOT EXISTS usertype_statistics ( timestamp INTEGER, type TEXT, count INTEGER, PRIMARY KEY (timestamp, type) );");

	for (timestamp_usertype_map_t::const_iterator i = usertype_maps.begin(); i != usertype_maps.end(); i++)
	{
		timestamp = i->first;
		usertype_map_t usertype_map = i->second;

		for (usertype_map_t::const_iterator j = usertype_map.begin(); j != usertype_map.end(); j++)
		{
			char *type = j->first;
			long unsigned int count = j->second;

			sql_query("INSERT OR IGNORE INTO usertype_statistics ( timestamp, type, count ) VALUES ( %ld, '%s', 0 );", timestamp, type);
			sql_query("UPDATE usertype_statistics SET count = count + %lu WHERE timestamp = %ld AND type = '%s';", count, timestamp, type);

			#ifdef __DEBUG__
				printf("udploggerstats.cc debug: usertype_maps[%ld].%s => %lu\n", timestamp, type, count);
			#endif
		}
	}
}
