#ifndef __BEACON_H__
#define __BEACON_H__

#include <netinet/in.h>

void *beacon_main(void *);
void expire_log_targets(void);
void receive_beacon(struct sockaddr_in *);
void receive_beacons(int);

#endif
