#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "beacon.h"

/**
 * acknowledge_beacon
 *
 * Takes the current list of log targets and a beacon source, updating the list of log targets
 * with the information in the beacon.
 **/
struct log_target *acknowledge_beacon(struct log_target *head, struct sockaddr_in *beacon_source)
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
