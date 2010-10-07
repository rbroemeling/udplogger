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

#ifndef __UDPLOGGERD_H__
#define __UDPLOGGERD_H__


/*
 * Structure that contains configuration information for the running instance
 * of udplogger.
 */
struct udploggerd_configuration_t {
	uint16_t listen_port;
	uintmax_t maximum_target_age;
	long prune_target_interval;
	char *tag;
	uint8_t tag_length;
};


/*
 * Structure that is used to store information about the logging targets that
 * the running instance of udplogger is currently sending log data to.  One host
 * per structure, arranged as a singly-linked list.
 */
struct log_target_t {
	time_t beacon_timestamp;
	struct sockaddr_in address;
	struct log_target_t *next;
};

/* Global Variables (see udploggerd.c). */
extern struct udploggerd_configuration_t conf;
extern struct log_target_t *targets;
extern pthread_mutex_t targets_mutex;


#endif
