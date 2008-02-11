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
			if (FD_ISSET(fd, &read_set))
			{
				receive_beacons(fd);
			}
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
struct log_target_t *expire_log_targets(struct log_target_t *head, uintmax_t max_age)
{
	struct log_target_t *current;
	struct log_target_t *previous;
	struct log_target_t *tmp;
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


void receive_beacon(struct sockaddr_in *beacon_source)
{
	struct log_target_t *current;
	
	/**
	 * Check to see if we currently know about this target, and if we do update it's beacon timestamp and then return.
	 *
	 * Note that we do not lock during this initial check.  This is because the updated field (beacon_timestamp) is only
	 * ever touched (read or written) by this thread.  If it is ever modified or read in another thread, this will need to lock.
	 **/
	current = targets;
	while (current)
	{
		if ((beacon_source->sin_addr.s_addr == current->address.sin_addr.s_addr) && (beacon_source->sin_port == current->address.sin_port))
		{
			current->beacon_timestamp = time(NULL);
			return;
		}
		current = current->next;
	}
	
	// This is a new target, so add it to our list and then return.
	current = calloc(1, sizeof(struct log_target_t));
	if (! current)
	{
		perror("calloc(1, sizeof(struct log_target_t))");
	}
	else
	{
		current->address.sin_family = AF_INET;
		current->address.sin_addr.s_addr = beacon_source->sin_addr.s_addr;
		current->address.sin_port = beacon_source->sin_port;
		current->beacon_timestamp = time(NULL);

		pthread_mutex_lock(&targets_mutex);
		current->next = targets;
		targets = current;
		pthread_mutex_unlock(&targets_mutex);
	}
}


void receive_beacons(int fd)
{
	struct sockaddr_in source;
	socklen_t source_len;
	unsigned char data[BEACON_PACKET_SIZE];
	ssize_t data_len;
	
	source_len = sizeof(source);
	data_len = recvfrom(fd, data, BEACON_PACKET_SIZE, 0, (struct sockaddr *) &source, &source_len);
	if (data_len > 0)
	{
		if (strncmp((char *)data, BEACON_STRING, BEACON_PACKET_SIZE) == 0)
		{
			head = receive_beacon(&source);
		}
	}
}
