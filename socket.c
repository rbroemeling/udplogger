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

#include <fcntl.h>
#include <inttypes.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>

/**
 * bind_socket(<port>, <blocking flag>)
 *
 * Utility function that creates a UDP socket bound to the given port and returns the
 * file descriptor for it.  The descriptor that is returned has SO_BROADCAST and SO_REUSEADDR
 * set.  If the blocking flag is zero then the descriptor returned is non-blocking.
 *
 * On error returns a negative value representing the error code encountered.
 **/
int bind_socket(uint16_t listen_port, uint16_t blocking)
{
	int fd;
	int result;
	unsigned int yes = 1;
	struct sockaddr_in listen_addr;
	
	fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (fd < 0)
	{
		perror("socket.c socket()");
		return fd;
	}
	
	result = setsockopt(fd, SOL_SOCKET, SO_BROADCAST, &yes, sizeof(yes));
	if (result < 0)
	{
		perror("socket.c setsockopt(SO_BROADCAST)");
		return result;
	}
	
	result = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
	if (result < 0)
	{
		perror("socket.c setsockopt(SO_REUSEADDR)");
		return result;
	}

	if (blocking == 0)
	{
		result = fcntl(fd, F_SETFL, O_NONBLOCK);
		if (result == -1)
		{
			perror("socket.c fcntl()");
			return result;
		}
	}

	memset(&listen_addr, 0, sizeof(listen_addr));
	listen_addr.sin_family = AF_INET;
	listen_addr.sin_port = htons(listen_port);
	listen_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	result = bind(fd, (struct sockaddr *) &listen_addr, sizeof(listen_addr));
	if (result < 0)
	{
		perror("socket.c bind()");
		return result;
	}
	
#ifdef __DEBUG__
	printf("socket.c debug: created fd %d, bound to port %hu.\n", fd, listen_port);
#endif
	return fd;
}
