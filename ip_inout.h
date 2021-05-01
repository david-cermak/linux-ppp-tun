#ifndef _IP_INOUT_H_
#define _IP_INOUT_H_

#include "lwip/netif.h"

int tun_init(char *devname);
int tun_read(int tun_fd, struct netif *pppos_netif);
void tun_deinit(int fd);

#endif //_IP_INOUT_H_
