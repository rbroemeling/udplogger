/**
 * Multicast UDP Logger Cat
 * the serialization is still broken, the packetid and pid do not get processed right
 **/
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <zlib.h>

#define LOGGER_PORT 12345
#define LOGGER_GROUP "225.0.0.37"
#define LOGGER_BUFSIZE (1024)

// this should not be modified (it includes the zlib buffer size)
// Packet is:
//	[int size][int pid][int id][char[size] zlib data]
#define LOGGER_ZBUFSZ (int)((LOGGER_BUFSIZE * 1.1) + 12)
#define LOGGER_HDRSZ (sizeof(int) + sizeof(unsigned long))
#define LOGGER_RECVBUF (LOGGER_HDRSZ + LOGGER_ZBUFSZ)

int main (int argc, char **argv) {
	/**
	 * should add command line parsing
	 **/
	struct sockaddr_in addr;
	int fd, rv, nbytes, addrlen, size;
	unsigned int pid;
	struct ip_mreq mreq;
	char linebuf[LOGGER_BUFSIZE];
	char recvbuf[LOGGER_RECVBUF];
	unsigned long buflen, cnt;
	unsigned int yes = 1;
	char *r;

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
	memset(recvbuf, 0, LOGGER_RECVBUF);

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
		if ((nbytes=recvfrom(fd,recvbuf,LOGGER_RECVBUF,0,(struct sockaddr *) &addr, &addrlen)) >= 0) {
			r = recvbuf;
			size = nbytes - LOGGER_HDRSZ;
			pid = *((unsigned int *)r); r += sizeof(unsigned int);
			cnt = *((unsigned long *)r); r += sizeof(unsigned long);
			buflen = LOGGER_BUFSIZE;
			rv = uncompress(linebuf, &buflen, r, size);

			if (rv == Z_OK)
				printf("[%s] [id=%lu] [pid=%u] %s\n", inet_ntoa(addr.sin_addr), cnt, pid, linebuf);
		}
	}
}
