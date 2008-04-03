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
 * On error returns a negative file descriptor representing the error code encountered.
 **/
int bind_socket(uint16_t listen_port, uint16_t blocking)
{
	int fd;
	int result;
	unsigned int yes = 1;
	struct sockaddr_in listen_addr;
	
	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0)
	{
		perror("socket.c socket(SOCK_DGRAM)");
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
			perror("socket.c fcntl(O_NONBLOCK)");
			return result;
		}
	}

	memset(&listen_addr, 0, sizeof(listen_addr));
	listen_addr.sin_family = AF_INET;
	listen_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	listen_addr.sin_port = htons(listen_port);

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
