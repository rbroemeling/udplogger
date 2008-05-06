#include <arpa/inet.h>
#include <getopt.h>
#include <stdio.h>
#include <string.h>
#include "udplogger.h"
#include "udploggerclientlib.h"


int add_option_hook();
int getopt_hook(char);
void inline log_packet_hook(struct sockaddr_in *, char *);
void usage_hook();


int add_option_hook()
{
	return 1;
}


int getopt_hook(char i)
{
	return 0;
}


void inline log_packet_hook(struct sockaddr_in *sender, char *line)
{
	static struct log_entry_t log_data;

	bzero(&log_data, sizeof(log_data));
	parse_log_line(line, &log_data);
}


void usage_hook()
{

}
