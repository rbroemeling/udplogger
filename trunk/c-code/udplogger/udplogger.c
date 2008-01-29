/**
 * UDP Logger
 **/
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <zlib.h>
#include "udplogger.h"

#define LOGGER_PORT 12345
#define LOGGER_GROUP "225.0.0.37"


size_t trim(unsigned char *buf)
{
	size_t i;

	for (i = strlen((char *)buf) - 1; i >= 0; i--)
	{
		if (! isspace(buf[i]))
		{
			break;
		}
	}
	i++;
	buf[i] = '\0';
	return i;
}


int main (int argc, char **argv)
{
	/**
	 * should add command line parsing
	 **/
	struct sockaddr_in addr;
	int fd, rv, size;
	LOGGER_PID_T pid;
	LOGGER_CNT_T cnt;
	unsigned char linebuf[LOGGER_BUFSIZE];
	unsigned char sendbuf[LOGGER_SENDBUF];
	unsigned long buflen;
	unsigned char *w;

	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		perror("socket()");
		return 255;
	}

	if (fcntl(fd, F_SETFL, O_NONBLOCK) == -1)
	{
		perror("fcntl(F_SETFL, O_NONBLOCK)");
	}

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(LOGGER_GROUP);
	addr.sin_port = htons(LOGGER_PORT);

	memset(linebuf, 0, LOGGER_BUFSIZE);
	memset(sendbuf, 0, LOGGER_SENDBUF);

	pid = (LOGGER_PID_T)getpid();
	cnt = 0;

	while (fgets((char *)linebuf, LOGGER_BUFSIZE, stdin) != NULL)
	{
		buflen = trim(linebuf);
		++cnt;
		w = sendbuf;
		memcpy(w, &pid, sizeof(pid)); w += sizeof(pid);
		memcpy(w, &cnt, sizeof(cnt)); w += sizeof(cnt);
		size = sizeof(sendbuf) - sizeof(pid) - sizeof(cnt);
		rv = compress2((Bytef *)w, (uLongf *)&size, linebuf, buflen+1, LOGGER_COMPRESSLEVEL);
		if (rv == Z_OK)
		{
			sendto(fd, sendbuf, (size + LOGGER_HDRSZ), 0, (struct sockaddr *) &addr, sizeof(addr));
		}
	}

	return 0;
}
