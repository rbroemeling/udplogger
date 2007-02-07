/**
 * Multicast UDP Logger Cat
 **/
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <zlib.h>
#include "udplogger.h"

#define LOGGER_PORT 12345
#define LOGGER_GROUP "225.0.0.37"
#define LOGGER_BUFSIZE (1024)

int main (int argc, char **argv) {
	/**
	 * should add command line parsing
	 **/
	struct sockaddr_in addr;
	int fd, rv, nbytes, addrlen, size;
	LOGGER_PID_T pid;
	LOGGER_CNT_T cnt;

	struct ip_mreq mreq;
	char linebuf[LOGGER_BUFSIZE];
	char recvbuf[LOGGER_SENDBUF];
	unsigned long buflen;
	unsigned int yes = 1;
	void *r;

	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("socket()");
		return 255;
	}

	if (setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(yes)) < 0) {
		perror("setsockopt(SOL_SOCKET,SO_REUSEADDR)");
		return 255;
	}

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(LOGGER_PORT);

	memset(linebuf, 0, LOGGER_BUFSIZE);
	memset(recvbuf, 0, LOGGER_SENDBUF);

	if (bind(fd,(struct sockaddr *) &addr,sizeof(addr)) < 0) {
		perror("bind()");
		return 255;
	}

	mreq.imr_multiaddr.s_addr=inet_addr(LOGGER_GROUP);
	mreq.imr_interface.s_addr=htonl(INADDR_ANY);

	if (setsockopt(fd,IPPROTO_IP,IP_ADD_MEMBERSHIP,&mreq,sizeof(mreq)) < 0) {
		perror("setsockopt()");
		return 255;
	}

	while (1) {
		addrlen = sizeof(addr);
		if ((nbytes=recvfrom(fd,recvbuf,sizeof(recvbuf),0,(struct sockaddr *) &addr, &addrlen)) >= 0) {
			r = recvbuf;
			size = nbytes - LOGGER_HDRSZ;
			memcpy(&pid, r, sizeof(pid)); r += sizeof(pid);
			memcpy(&cnt, r, sizeof(cnt)); r += sizeof(cnt);
			buflen = LOGGER_BUFSIZE;
			rv = uncompress(linebuf, &buflen, r, size);

			if (rv == Z_OK)
				printf("[%s] [id=%lu] [pid=%u] %s\n", inet_ntoa(addr.sin_addr), cnt, pid, linebuf);
		}
	}
}
