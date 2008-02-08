#ifndef __BEACON_H__
#define __BEACON_H__

#include <netinet/in.h>

struct log_target_t {
	time_t beacon_timestamp;
	struct sockaddr_in address;
	struct log_target_t *next;
};

struct log_target_t *expire_log_targets(struct log_target_t *, uintmax_t);
struct log_target_t *receive_beacon(struct log_target_t *, int);
struct log_target_t *update_beacon(struct log_target_t *, struct sockaddr_in *);

#endif //!__BEACON_H__
