#ifndef __BEACON_H__
#define __BEACON_H__

#include <netinet/in.h>

struct log_target {
	time_t beacon_timestamp;
	struct sockaddr_in address;
	struct log_target *next;
};

struct log_target *acknowledge_beacon(struct log_target *, struct sockaddr_in *);
struct log_target *expire_log_targets(struct log_target *, uintmax_t);

#endif //!__BEACON_H__
