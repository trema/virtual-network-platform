/*
 * Copryright (C) 2012 upa@haeena.net
 * Copryright (C) 2012-2013 NEC Corporation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2, as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
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
