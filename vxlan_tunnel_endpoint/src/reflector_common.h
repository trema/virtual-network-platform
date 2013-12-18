/*
 * Author: Yasunobu Chiba
 *
 * Copyright (C) 2012-2013 NEC Corporation
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


#ifndef REFLECTOR_COMMON_H
#define REFLECTOR_COMMON_H


#include <netinet/ether.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/select.h>
#include "queue.h"
#include "vxlan.h"
#include "wrapper.h"


#define PACKET_SIZE 9000


enum {
  SUCCEEDED = 0,
  INVALID_ARGUMENT = 1,
  ALREADY_RUNNING = 2,
  PORT_ALREADY_IN_USE = 3,
  DUPLICATED_TEP_ENTRY = 4,
  TEP_ENTRY_NOT_FOUND = 5,
  OTHER_ERROR = 255,
};


typedef struct {
  uint32_t vni;
  struct ether_addr eth_addr;
  struct in_addr ip_addr;
  uint16_t port;
  struct {
    uint64_t packet;
    uint64_t octet;
  } counters;
} tunnel_endpoint;

typedef struct {
  char data[ PACKET_SIZE + 1 ];
  size_t length;
  struct iphdr *ip;
  struct udphdr *udp;
  struct vxlanhdr *vxlan;
} packet_buffer;


extern pthread_t receiver_thread;
extern pthread_t distributor_thread;
extern pthread_mutex_t mutex;
extern pthread_cond_t cond;
extern queue *received_packets;
extern queue *free_packet_buffers;
extern volatile bool running;


#endif // REFLECTOR_COMMON_H


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
