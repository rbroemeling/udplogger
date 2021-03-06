/**
 * The MIT License (http://www.opensource.org/licenses/mit-license.php)
 * 
 * Copyright (c) 2010 Nexopia.com, Inc.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 **/

#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <time.h>
#include "udplogger.h"
#include "beacon.h"
#include "socket.h"
#include "udploggerd.h"


/**
 * beacon_main(<fd_ptr>)
 *
 * The main thread loop for the beacon-listening thread.  Creates a socket to listen on
 * and then enters a loop waiting for input.  Beacons are expired/cleaned at the end of each
 * loop; which means that they are cleaned whenever beacon packet(s) are received or when the
 * select timeout elapses (which upper-bounds how often the list of targets is cleaned).
 **/
void *beacon_main(void *fd_ptr)
{
	fd_set all_set;
	int fd = *(int *)fd_ptr;
	fd_set read_set;
	int result = 0;
	struct timeval timeout;
	
	/*
	 * Configure our select timeout -- this is used to control the time
	 * between prunes of the target list.
	 */
	timeout.tv_sec = conf.prune_target_interval;
	timeout.tv_usec = 0L;
	
	FD_ZERO(&all_set);
	FD_SET(fd, &all_set);
	while (1)
	{
		read_set = all_set;
		result = select(fd+1, &read_set, NULL, NULL, &timeout);
		if (result < 0)
		{
			perror("beacon.c select()");
			pthread_exit(NULL);
		}
		else if (result == 0)
		{
			timeout.tv_sec = conf.prune_target_interval;
			timeout.tv_usec = 0L;
			expire_log_targets();
		}
		else if (result > 0)
		{
			if (FD_ISSET(fd, &read_set))
			{
				receive_beacons(fd);
			}
		}
	}
	pthread_exit(NULL);
}


/**
 * expire_log_targets()
 *
 * Garbage-collector function for the list of log targets.  Loops through the list and
 * removes any that have expired (i.e. are older than conf.maximum_target_age).
 **/
void expire_log_targets()
{
	struct log_target_t *current = NULL;
	struct log_target_t *previous = NULL;
	struct log_target_t *tmp = NULL;
	time_t minimum_timestamp = 0;
	
	time(&minimum_timestamp);
	if (minimum_timestamp == -1)
	{
		perror("beacon.c time()");
		return;
	}
	minimum_timestamp = minimum_timestamp - conf.maximum_target_age;

#ifdef __DEBUG__
	printf("beacon.c debug: expiring log targets older than %lu\n", minimum_timestamp);
#endif

	current = targets;
	while (current)
	{
		if (current->beacon_timestamp < minimum_timestamp)
		{
			printf("beacon.c debug: removing target %s:%hu, expired at %lu\n", inet_ntoa(current->address.sin_addr), ntohs(current->address.sin_port), current->beacon_timestamp);
			if (current == targets)
			{
				pthread_mutex_lock(&targets_mutex);
				targets = current->next;
				pthread_mutex_unlock(&targets_mutex);
			}
			if (previous)
			{
				pthread_mutex_lock(&targets_mutex);
				previous->next = current->next;
				pthread_mutex_unlock(&targets_mutex);
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
}


/**
 * receive_beacon(<beacon source>)
 *
 * Utility function to take the source of a beacon and update the list of targets.  If the source of the beacon
 * is already in the list of targets, the timestamp of the last beacon from that host is updated to the current time.
 * If the source of the beacon is not yet in the list of targets, it is added to the end of the list and the current time
 * is taken as the time of it's receipt.
 **/
void receive_beacon(struct sockaddr_in *beacon_source)
{
	struct log_target_t *current = NULL;
	
#ifdef __DEBUG__
	printf("beacon.c debug: beacon received from %s:%hu\n", inet_ntoa(beacon_source->sin_addr), ntohs(beacon_source->sin_port));
#endif

	/*
	 * Check to see if we currently know about this target, and if we do update its beacon timestamp and then return.
	 *
	 * Note that we do not lock during this check.  This is because the updated field (beacon_timestamp) is only
	 * ever touched (read or written) by this thread.  If it is ever modified or read in another thread, this will need to lock
	 * while it carries out the update.
	 */
	current = targets;
	while (current)
	{
		if ((beacon_source->sin_addr.s_addr == current->address.sin_addr.s_addr) && (beacon_source->sin_port == current->address.sin_port))
		{
			current->beacon_timestamp = time(NULL);
#ifdef __DEBUG__
			printf("beacon.c debug: updated expiry of target %s:%hu to %lu\n", inet_ntoa(beacon_source->sin_addr), ntohs(beacon_source->sin_port), current->beacon_timestamp);
#endif
			return;
		}
		current = current->next;
	}
	
	/* This is a new target, so add it to our list and then return. */
	current = calloc(1, sizeof(struct log_target_t));
	if (! current)
	{
		perror("beacon.c calloc()");
	}
	else
	{
		current->address.sin_family = AF_INET;
		current->address.sin_addr.s_addr = beacon_source->sin_addr.s_addr;
		current->address.sin_port = beacon_source->sin_port;
		current->beacon_timestamp = time(NULL);

#ifdef __DEBUG__
		printf("beacon.c debug: added target %s:%hu with expiry %lu\n", inet_ntoa(current->address.sin_addr), ntohs(current->address.sin_port), current->beacon_timestamp);
#endif

		pthread_mutex_lock(&targets_mutex);
		current->next = targets;
		targets = current;
		pthread_mutex_unlock(&targets_mutex);
	}
}


/**
 * receive_beacons(<incoming socket>)
 *
 * Utility function that is called when the socket passed in has messages waiting for delivery.
 * Reads all of the available packets from the socket and passes the source address of each one
 * to receive_beacon if the packet data appears to be a udplogger beacon.
 **/
void receive_beacons(int fd)
{
	struct sockaddr_in source;
	socklen_t source_len;
	unsigned char data[BEACON_PACKET_SIZE];
	ssize_t data_len;
	
	source_len = sizeof(source);
	data_len = recvfrom(fd, data, BEACON_PACKET_SIZE, 0, (struct sockaddr *) &source, &source_len);
#ifdef __DEBUG__
	printf("beacon.c debug: received packet from %s:%hu\n", inet_ntoa(source.sin_addr), ntohs(source.sin_port));
#endif
	if (data_len > 0)
	{
		if (strncmp((char *)data, BEACON_STRING, BEACON_PACKET_SIZE) == 0)
		{
			receive_beacon(&source);
		}
	}
}
