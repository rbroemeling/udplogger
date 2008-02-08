#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "beacon.h"
#include "udplogger.h"


void *beacon_main(void *arg)
{
	fd_set all_set;
	int fd;
	fd_set read_set;
	int result = 0;
	struct timeval timeout;

	// Set up the socket that will be used to listen for beacons.
	fd = bind_socket(conf.listen_port);
	if (fd < 0)
	{
		fprintf(stderr, "could not setup beacon-listener socket\n");
		pthread_exit(NULL);
	}
	
	// Configure our select timeout -- this is used to control the maximum time
	// between prunes of the target list.
	timeout.tv_sec = conf.prune_target_maximum_interval;
	timeout.tv_usec = 0L;
	
	FD_ZERO(&all_set);
	FD_SET(fd, &all_set);
	while (1)
	{
		read_set = all_set;
		result = select(1, &read_set, NULL, NULL, &timeout);
		if (result < 0)
		{
			perror("beacon-listener select");
			pthread_exit(NULL);
		}
		if (result > 0)
		{
		
		}
		targets = expire_log_targets(targets, conf.maximum_target_age);
	}
	pthread_exit(NULL);
}

/**
 * expire_log_targets
 *
 * Takes the current list of log targets, the maximum age that they should be (in seconds),
 * and returns a pruned list of log targets with expired entries removed.
 **/
struct log_target *expire_log_targets(struct log_target *head, uintmax_t max_age)
{
	struct log_target *current;
	struct log_target *previous;
	struct log_target *tmp;
	time_t minimum_timestamp = time(NULL) - max_age;
	
	current = head;
	previous = NULL;
	while (current)
	{
		if (current->beacon_timestamp < minimum_timestamp)
		{
			if (current == head)
			{
				head = current->next;
			}
			if (previous)
			{
				previous->next = current->next;
			}
			tmp = current;
			current = current->next;
			free(tmp);
		}
		else
		{
			previous = current;
			current = current->next;
		}
	}
	return head;
}


struct log_target *receive_beacon(struct log_target *head, int fd)
{
	struct sockaddr_in source;
	socklen_t source_len;
	unsigned char data[LOG_PACKET_SIZE];
	ssize_t data_len;
	
	source_len = sizeof(source);
	data_len = recvfrom(fd, data, sizeof(data), 0, (struct sockaddr *) &source, &source_len);
	if (strncmp((char *)data, BEACON_STRING, LOG_PACKET_SIZE) == 0)
	{
		head = update_beacon(head, &source);
	}
	return head;
}


/**
 * update_beacon
 *
 * Takes the current list of log targets and a beacon source, updating the list of log targets
 * with the information in the beacon.
 **/
struct log_target *update_beacon(struct log_target *head, struct sockaddr_in *beacon_source)
{
	struct log_target *current;
	
	// Check to see if we currently know about this target, and if we do update it's beacon timestamp and then return.
	current = head;
	while (current)
	{
		if ((beacon_source->sin_addr.s_addr == current->address.sin_addr.s_addr) && (beacon_source->sin_port == current->address.sin_port))
		{
			current->beacon_timestamp = time(NULL);
			return head;
		}
		current = current->next;
	}
	
	// This is a new target, so add it to our list and then return.
	current = calloc(1, sizeof(struct log_target));
	if (! current)
	{
		perror("calloc(1, sizeof(struct log_target))");
	}
	else
	{
		current->address.sin_family = AF_INET;
		current->address.sin_addr.s_addr = beacon_source->sin_addr.s_addr;
		current->address.sin_port = beacon_source->sin_port;
		current->beacon_timestamp = time(NULL);
		current->next = head;
		head = current;
	}

	return head;
}
