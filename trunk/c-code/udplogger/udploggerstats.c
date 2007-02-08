/**
 * Multicast UDP Log Parser
 **/
#include <stdlib.h>
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
#include "udploggerstats.h"

#define LOGGER_PORT 12345
#define LOGGER_GROUP "225.0.0.37"
#define LOGGER_BUFSIZE (1024)

#define LOGGER_FMT "%m %>s %B %T \"%U\" \"%q\" \"%a\" \"%{blah}o\" \"%{X-Forwarded-For}i\" \"%{X-LIGHTTPD-userid}o\" \"%{X-LIGHTTPD-age}o\" \"%{X-LIGHTTPD-sex}o\" \"%{X-LIGHTTPD-loc}o\" \"%{X-LIGHTTPD-usertype}o\""

int parse_line(hit_t *hit, char *line, const char *fmt);
int print_hit(hit_t *hit);

int main (int argc, char **argv) {
	/**
	 * should add command line parsing
	 **/
	struct sockaddr_in addr;
	int fd, rv, nbytes, size, i;
	socklen_t addrlen;

	LOGGER_PID_T pid;
	LOGGER_CNT_T cnt;
	hit_t hit;

	char *fmt = LOGGER_FMT;

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

	memset(&hit, 0, sizeof(hit));

	while (1) {
		addrlen = sizeof(addr);
		if ((nbytes=recvfrom(fd,recvbuf,sizeof(recvbuf),0,(struct sockaddr *) &addr, &addrlen)) >= 0) {
			r = recvbuf;
			size = nbytes - LOGGER_HDRSZ;
			memcpy(&pid, r, sizeof(pid)); r += sizeof(pid);
			memcpy(&cnt, r, sizeof(cnt)); r += sizeof(cnt);
			buflen = LOGGER_BUFSIZE;
			rv = uncompress((Bytef *)linebuf, &buflen, r, size);

			if (rv == Z_OK) {
				memset(&hit, 0, sizeof(hit));
				hit.addr = &addr;
				hit.cnt = cnt;
				hit.pid = pid;
				printf("[parse rv=%i] ", parse_line(&hit, linebuf, fmt));
				print_hit(&hit); // debug


				/**
				 * right now keys are strdup'd, so we have to free them
				 * in the future, it'd be nice to have all keys allocated once
				 * and not be re-allocated.
				 **/
				for (i = 0; i < HIT_MAX_HEADERS; ++i)
					if (hit.headers[i].key != NULL) {
						free(hit.headers[i].key);
						hit.headers[i].key = NULL;
					} 
			}
		}
	}
}

/**
 * Converts a string to one of the protocol enums
 **/
http_protocol str2protocol(const char *str) {
	if (!str || *str == '\0')
		return HTTP_UNKNOWN_PROTOCOL;
	else if (strcmp(str, "HTTP/1.1") == 0)
		return HTTP_1_1;
	else if (strcmp(str, "HTTP/1.0") == 0)
		return HTTP_1_0;
	else if (strcmp(str, "HTTP/0.9") == 0)
		return HTTP_0_9;
	else
		return HTTP_EXTENSION_PROTOCOL;
}

/**
 * Converts a string to one of the http method enums
 * This is in oogly tbone micro-optimized notation
 * Although unreadable, it is faster then if/else trees
 **/
http_method str2method(const char *str) {
	const char *p;
	if (!str || *str == '\0')
		return HTTP_UNKNOWN_METHOD;

	p = str;
	switch (*p) {
		case 'G':
			++p; return (strcmp(p, "ET") == 0)?HTTP_GET:HTTP_EXTENSION_METHOD;
		case 'H':
			++p; return (strcmp(p, "EAD") == 0)?HTTP_HEAD:HTTP_EXTENSION_METHOD;
		case 'P':
			++p; switch (*p) {
				case 'O':
					++p; return (strcmp(p, "ST") == 0)?HTTP_POST:HTTP_EXTENSION_METHOD;
				case 'U':
					++p; return (*p != '\0' && (*p == 'T' && *(p+1) == '\0'))?HTTP_PUT:HTTP_EXTENSION_METHOD;
			}
			return HTTP_EXTENSION_METHOD;
		case 'O':
			++p; return (strcmp(p, "PTIONS") == 0)?HTTP_OPTIONS:HTTP_EXTENSION_METHOD;
		case 'T':
			++p; return (strcmp(p, "RACE") == 0)?HTTP_TRACE:HTTP_EXTENSION_METHOD;
		case 'C':
			++p; return (strcmp(p, "ONNECT") == 0)?HTTP_CONNECT:HTTP_EXTENSION_METHOD;
		case 'D':
			++p; return (strcmp(p, "ELETE") == 0)?HTTP_DELETE:HTTP_EXTENSION_METHOD;
	}
	return HTTP_EXTENSION_METHOD;
}

int parse_line(hit_t *hit, char *line, const char *fmt) {
	char *wp, *tmp;
	const char *rp;
	int hpos;
	char hdrbuf[HIT_MAX_HEADERS];
	rp = fmt;
	wp = line;
	hpos = 0;

	while (*rp != '\0') {
		if (*rp == '%') {
			++rp;
			switch (*rp) {
				case 'h':
					hit->raw_remote_host = wp;
					break;
				case 'u':
					hit->raw_auth_user = wp;
					break;
				case 't':
					hit->raw_request_start = wp;
					break;
				case 'r':
					hit->raw_request_line = wp;
					break;
				case 'b':
				case 'B':
					hit->raw_body_size = wp;
					break;
				case 'a':
					hit->raw_remote_address = wp;
					break;
				case 'A':
					hit->raw_local_address = wp;
					break;
				case 'f':
					hit->raw_filename = wp;
					break;
				case 'H':
					hit->raw_protocol = wp;
					break;
				case 'm':
					hit->raw_method = wp;
					break;
				case 'p':
					hit->raw_server_port = wp;
					break;
				case 'q':
					hit->raw_query = wp;
					break;
				case 'T':
					hit->raw_request_time = wp;
					break;
				case 'U':
					hit->raw_request_url = wp;
					break;
				case 'v':
					hit->raw_server_name = wp;
					break;
				case 'V':
					hit->raw_request_host = wp;
					break;
				case 'x':
					hit->raw_conn_status = wp;
					break;
				case 'I':
					hit->raw_bytes_in = wp;
					break;
				case 'O':
					hit->raw_bytes_out = wp;
					break;
				case '>':
				case '<':
					++rp;
					if ( *rp != 's' )
						break;
				case 's':
					hit->raw_status_code = wp;
					break;
				case '%':
					break;
				case '{':
					++rp;
					tmp = hdrbuf;
					while (*rp != '}' && *rp != '\0' && tmp != &hdrbuf[HIT_MAX_HEADERS-1]) {
						*tmp = *rp;
						++tmp;
						++rp;
					}
					++rp;
					*tmp = '\0';
					if (*rp == 'i' || *rp == 'o' || *rp == 'n') {
						hit->headers[hpos].key = strdup(hdrbuf);
						hit->headers[hpos].value = wp;
						++hpos;
					}
					break;
				default:
					break;

			}

			++rp;
			if (*rp != *wp) {
				++wp;
				while (*rp != *wp && *wp != '\0')
					wp++;
				if (*rp != *wp && *wp == '\0')
					return PARSE_ERR;
			}

			*wp = '\0';
			wp++;
			rp++;
		} else {
			if (*rp == *wp) {
				rp++;
				wp++;
				continue;
			} else {
				return PARSE_ERR;
			}
		}
	}

	if (hit->raw_method) hit->method = str2method(hit->raw_method);
	if (hit->raw_protocol) hit->protocol = str2protocol(hit->raw_protocol);
	if (hit->raw_local_address) hit->local_address = inet_addr(hit->raw_local_address);
	if (hit->raw_remote_address) hit->remote_address = inet_addr(hit->raw_remote_address);
	if (hit->raw_body_size) hit->body_size = (unsigned int)strtol(hit->raw_body_size, (char **)NULL, 10);
	if (hit->raw_server_port) hit->server_port = (unsigned short)strtol(hit->raw_server_port, (char **)NULL, 10);
	if (hit->raw_request_time) hit->request_time = (unsigned int)strtol(hit->raw_request_time, (char **)NULL, 10);
	if (hit->raw_bytes_in) hit->bytes_in = (unsigned int)strtol(hit->raw_bytes_in, (char **)NULL, 10);
	if (hit->raw_bytes_in) hit->bytes_out = (unsigned int)strtol(hit->raw_bytes_out, (char **)NULL, 10);

	return PARSE_OK; // need to add more error detection
}

int print_hit(hit_t *hit) {
	int i;
	printf("[%s] [id=%u] [pid=%u]\n", inet_ntoa(hit->addr->sin_addr), hit->cnt, hit->pid);
	printf("  remote_host:    '%s'\n", hit->raw_remote_host);
	printf("  auth_user:      '%s'\n", hit->raw_auth_user);
	printf("  request_start:  '%s'\n", hit->raw_request_start);
	printf("  request_line:   '%s'\n", hit->raw_request_line);
	printf("  status_code:    '%s'\n", hit->raw_status_code);
	printf("  body_size:      '%s'\n", hit->raw_body_size);
	printf("  remote_address: '%s'\n", hit->raw_remote_address);
	printf("  local_address:  '%s'\n", hit->raw_local_address);
	printf("  filename:       '%s'\n", hit->raw_filename);
	printf("  protocol:       '%s'\n", hit->raw_protocol);
	printf("  method:         '%s'\n", hit->raw_method);
	printf("  server_port:    '%s'\n", hit->raw_server_port);
	printf("  query:          '%s'\n", hit->raw_query);
	printf("  request_time:   '%s'\n", hit->raw_request_time);
	printf("  request_url:    '%s'\n", hit->raw_request_url);
	printf("  server_name:    '%s'\n", hit->raw_server_name);
	printf("  request_host:   '%s'\n", hit->raw_request_host);
	printf("  conn_status:    '%s'\n", hit->raw_conn_status);
	printf("  bytes_in:       '%s'\n", hit->raw_bytes_in);
	printf("  bytes_out:      '%s'\n", hit->raw_bytes_out);
	printf("  http_headers:\n");
	for (i = 0; i < HIT_MAX_HEADERS; ++i)
		if (hit->headers[i].key != NULL)
			printf("    '%s' : '%s'\n", hit->headers[i].key, hit->headers[i].value);
		else
			break;
	
	return 0;
}
