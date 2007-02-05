/**
 * UDP Logger (should work with multicast)
 * the serialization is still broken, the packetid and pid do not get processed right
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

#define LOGGER_PORT 12345
#define LOGGER_GROUP "225.0.0.37"
#define LOGGER_BUFSIZE (1024)
#define LOGGER_COMPRESSLEVEL 0

// this should not be modified (it includes the zlib buffer size)
// Packet is:
//	[int pid][long id][char[size] zlib data]
#define LOGGER_ZBUFSZ (int)((LOGGER_BUFSIZE * 1.1) + 12)
#define LOGGER_HDRSZ (sizeof(int) + sizeof(unsigned long))
#define LOGGER_SENDBUF (LOGGER_HDRSZ + LOGGER_ZBUFSZ)

unsigned long cleanline(unsigned char *buf) {
	unsigned char *r;
	unsigned long len = strlen((char *)buf);

	r = buf + len;
	while (*r == '\0' || *r == '\n' || *r == '\r') 
		--r;
	r++;
	*r = '\0';
	return strlen((char *)buf);
}

int main (int argc, char **argv) {
	/**
	 * should add command line parsing
	 **/
	struct sockaddr_in addr;
	int fd, rv, pid, size;
	unsigned char linebuf[LOGGER_BUFSIZE];
	unsigned char sendbuf[LOGGER_SENDBUF];
	unsigned long buflen, cnt;
	unsigned char *w;

	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("socket()");
		return 255;
	}

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(LOGGER_GROUP);
	addr.sin_port = htons(LOGGER_PORT);

	memset(linebuf, 0, LOGGER_BUFSIZE);
	memset(sendbuf, 0, LOGGER_SENDBUF);

	pid = (unsigned int)getpid();
	cnt = 0;
printf("%u\n", pid);
	while (fgets((char *)linebuf, LOGGER_BUFSIZE, stdin) != NULL) {
		buflen = cleanline(linebuf);
		++cnt;
		w = sendbuf;
		*(unsigned int *)w = (unsigned int)pid; w += sizeof(unsigned int);
		*(unsigned long *)w = (unsigned long)cnt; w += sizeof(unsigned long);
		size = sizeof(sendbuf) - sizeof(unsigned int) - sizeof(unsigned int) - sizeof(unsigned long);
		rv = compress2((Bytef *)w, (uLongf *)&size, linebuf, buflen+1, LOGGER_COMPRESSLEVEL);
		if (rv == Z_OK)
			sendto(fd, sendbuf, (size + LOGGER_HDRSZ), 0, (struct sockaddr *) &addr, sizeof(addr));
	}

	return 0;
}
