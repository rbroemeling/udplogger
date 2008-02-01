#ifndef __BEACON_H__
#define __BEACON_H__

#include <netinet/in.h>

struct log_target {
	time_t beacon_timestamp;
	struct sockaddr_in address;
	struct log_target *next;
};

struct log_target *expire_log_targets(struct log_target *, uintmax_t);
struct log_target *receive_beacon(struct log_target *, int);
void send_targets(struct log_target *, int, void *, size_t, int);
struct log_target *update_beacon(struct log_target *, struct sockaddr_in *);

#endif //!__BEACON_H__
