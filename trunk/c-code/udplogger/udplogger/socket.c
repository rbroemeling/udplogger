#include <fcntl.h>
#include <inttypes.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>

/**
 * bind_socket(<port>)
 *
 * Utility function that creates a UDP socket bound to the given port and returns the
 * file descriptor for it.  The descriptor that is returned is non-blocking and has
 * SO_REUSEADDR set.
 *
 * On error returns a negative file descriptor representing the error code encountered.
 **/
int bind_socket(uint16_t listen_port)
{
	int fd;
	int result;
	unsigned int yes = 1;
	struct sockaddr_in listen_addr;
	
	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0)
	{
		perror("socket.c socket()");
		return fd;
	}
	
	result = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
	if (result < 0)
	{
		perror("socket.c setsockopt()");
		return result;
	}

	result = fcntl(fd, F_SETFL, O_NONBLOCK);
	if (result == -1)
	{
		perror("socket.c fcntl()");
		return result;
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
