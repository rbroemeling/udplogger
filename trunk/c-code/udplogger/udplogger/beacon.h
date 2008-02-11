#ifndef __BEACON_H__
#define __BEACON_H__

#include <netinet/in.h>

struct log_target_t {
	time_t beacon_timestamp;
	struct sockaddr_in address;
	struct log_target_t *next;
};

void expire_log_targets(uintmax_t);
void receive_beacon(struct sockaddr_in *);
void receive_beacons(int);

#endif //!__BEACON_H__
