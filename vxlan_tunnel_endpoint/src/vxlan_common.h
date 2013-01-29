/*
 * Commonly-used constant definitions.
 *
 * Copryright (C) 2012 upa@haeena.net
 * Copryright (C) 2012-2013 NEC Corporation
 */


#ifndef VXLAN_COMMON_H
#define VXLAN_COMMON_H


#include <net/if.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/socket.h>
#include <sys/types.h>
#include "hash.h"
#include "wrapper.h"


#define VXLAN_PACKET_BUF_LEN 9216


enum {
  SUCCEEDED = 0,
  INVALID_ARGUMENT = 1,
  ALREADY_RUNNING = 2,
  PORT_ALREADY_IN_USE = 3,
  SOURCE_PORTS_NOT_ALLOCATED = 4,
  DUPLICATED_INSTANCE = 5,
  INSTANCE_NOT_FOUND = 6,
  OTHER_ERROR = 255,
};


struct vxlan {
  int udp_sock;
  int timerfd;
  bool active;
  char ifname[ IFNAMSIZ ];
  uint16_t port;
  struct in_addr flooding_addr;
  uint16_t flooding_port;
  time_t aging_time;
  int n_instances;
  struct hash instances;
  pthread_t control_tid;
  bool daemonize;
  uint8_t log_output;
};


extern volatile bool running;


#endif // VXLAN_COMMON_H


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
