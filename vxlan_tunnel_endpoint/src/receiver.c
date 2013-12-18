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


#include <assert.h>
#include <errno.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/types.h>
#include <unistd.h>
#include "checks.h"
#include "ethdev.h"
#include "hash.h"
#include "linked_list.h"
#include "log.h"
#include "receiver.h"
#include "reflector_common.h"
#include "queue.h"
#include "wrapper.h"


static void
notify_distributor() {
  pthread_mutex_lock( &mutex );

  if ( received_packets->length > 0 ) {
    pthread_cond_signal( &cond );
  }

  pthread_mutex_unlock( &mutex );
}


void *
receiver_main( void *args ) {
  assert( args != NULL );

  info( "Receiver thread is started ( pid = %u, tid = %u).",
        getpid(), receiver_thread );

  receiver_options *options = args;
  ethdev *dev = options->dev;
  assert( dev->fd >= 0 );
  packet_buffer trash;

  while ( running ) {
    notify_distributor();

    fd_set fds;
    FD_ZERO( &fds );
    FD_SET( dev->fd, &fds );
    struct timespec timeout = { 0, 1000000 };
    int ret = pselect( dev->fd + 1, &fds, NULL, NULL, &timeout, NULL );
    if ( ret < 0 ) {
      if ( errno == EINTR ) {
        continue;
      }
      char buf[ 256 ];
      char *error_string = safe_strerror_r( errno, buf, sizeof( buf ) );
      error( "Failed to select ( errno = %s [%d] ).", error_string, errno );
      break;
    }
    else if ( ret == 0 ) {
      continue;
    }

    if ( !FD_ISSET( dev->fd, &fds ) ) {
      continue;
    }

    packet_buffer *packet = peek( free_packet_buffers );
    if ( packet == NULL ) {
      // TODO: implement a counter to remember # of packet losses
      packet = &trash;
    }
    int err = 0;
    ssize_t length = recv_from_ethdev( dev, packet->data, sizeof( packet->data ), &err );
    if ( packet == &trash ) {
      continue;
    }

    if ( length < ( sizeof( struct iphdr ) + sizeof( struct udphdr ) + sizeof( struct vxlanhdr ) ) ||
         length > PACKET_SIZE ) {
      continue;
    }

    packet->ip = ( struct iphdr * ) packet->data;
    if ( packet->ip->protocol != IPPROTO_UDP ) {
      continue;
    }
    packet->udp = ( struct udphdr * ) ( ( char * ) packet->ip + ( packet->ip->ihl * 4 ) );

    if ( ntohs( packet->udp->dest ) != options->port ) {
      continue;
    }
    packet->vxlan = ( struct vxlanhdr * ) ( ( char * ) packet->udp + sizeof( struct udphdr ) );
    packet->length = ( size_t ) length;

    dequeue( free_packet_buffers );
    enqueue( received_packets, packet );
  }

  running = false;

  info( "Receiver thread is terminated ( pid = %u, tid = %u ).",
        getpid(), receiver_thread );

  free( options );

  return NULL;
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
