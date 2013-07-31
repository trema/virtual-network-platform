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
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/types.h>
#include <unistd.h>
#include "checks.h"
#include "ethdev.h"
#include "reflector_common.h"
#include "hash.h"
#include "linked_list.h"
#include "log.h"
#include "queue.h"
#include "wrapper.h"


static struct hash *tunnel_endpoints = NULL;


static void
create_tunnel_endpoints() {
  assert( tunnel_endpoints == NULL );

  tunnel_endpoints = malloc( sizeof( struct hash ) );
  assert( tunnel_endpoints != NULL );

  init_hash( tunnel_endpoints, sizeof( uint32_t ) );
}


static void
delete_tunnel_endpoints() {
  assert( tunnel_endpoints != NULL );

  list *vnis = create_list();

  for ( int n = 0; n < HASH_TABLE_SIZE; n++ ) {
    struct hashnode *ptr = &tunnel_endpoints->table[ n ];
    pthread_mutex_lock( &tunnel_endpoints->mutex[ n ] );
    for ( struct hashnode *p = ptr; p != NULL; p = p->next ) {
      if ( p->key != NULL ) {
        append_to_tail( vnis, p->key );
      }
    }
    pthread_mutex_unlock( &tunnel_endpoints->mutex[ n ] );
  }

  for ( list_element *e = vnis->head; e != NULL; e = e->next ) {
    if ( e->data != NULL ) {
      void *deleted = delete_hash( tunnel_endpoints, e->data );
      if ( deleted != NULL ) {
        delete_list_totally( deleted );
      }
    }
  }
  delete_list( vnis );

  destroy_hash( tunnel_endpoints );
  free( tunnel_endpoints );
  tunnel_endpoints = NULL;
}


list *
lookup_tunnel_endpoints( uint32_t vni ) {
  assert( tunnel_endpoints != NULL );

  list *l = ( list * ) search_hash( tunnel_endpoints, &vni );
  if ( l == NULL || ( l != NULL && l->head == NULL ) ) {
    return NULL;
  }

  return l;
}


list *
get_all_tunnel_endpoints() {
  assert( tunnel_endpoints != NULL );

  int n = 0;
  list **lists = ( list ** ) create_list_from_hash( tunnel_endpoints, &n );
  if ( n == 0 || lists == NULL ) {
    if ( lists != NULL ) {
      free( lists );
    }
    return NULL;
  }

  list *endpoints = create_list();
  for ( int i = 0; i < n; i++ ) {
    pthread_mutex_lock( &( lists[ i ]->mutex ) );
    for ( list_element *e = lists[ i ]->head; e != NULL; e = e->next ) {
      if ( e->data != NULL ) {
        tunnel_endpoint *tep = malloc( sizeof( tunnel_endpoint ) );
        memcpy( tep, e->data, sizeof( tunnel_endpoint ) );
        append_to_tail( endpoints, tep );
      }
    }
    pthread_mutex_unlock( &( lists[ i ]->mutex ) );
  }

  free( lists );

  return endpoints;
}


bool
add_tunnel_endpoint( uint32_t vni, struct in_addr ip_addr, uint16_t port ) {
  assert( tunnel_endpoints != NULL );

  if ( !valid_vni( vni ) ) {
    return false;
  }

  tunnel_endpoint *tep = malloc( sizeof( tunnel_endpoint ) );
  assert( tep != NULL );
  memset( tep, 0, sizeof( tunnel_endpoint ) );
  tep->vni = vni;
  memcpy( &tep->ip_addr, &ip_addr, sizeof( tep->ip_addr ) );
  tep->port = htons( port );

  list *l = ( list * ) search_hash( tunnel_endpoints, &vni );
  if ( l == NULL ) {
    l = create_list();
    assert( l != NULL );
    int ret = insert_hash( tunnel_endpoints, l, &vni );
    if ( ret < 0 ) {
      error( "Failed to insert a hash entry." );
      free( tep );
      return false;
    }
  }

  bool duplicated = false;
  pthread_mutex_lock( &l->mutex );
  for ( list_element *e = l->head; e != NULL; e = e->next ) {
    if ( e->data != NULL ) {
      tunnel_endpoint *t = e->data;
      if ( t->ip_addr.s_addr == ip_addr.s_addr ) {
        duplicated = true;
        break;
      }
    }
  }
  if ( !duplicated ) {
    append_to_tail( l, tep );
  }
  else {
    free( tep );
  }
  pthread_mutex_unlock( &l->mutex );

  return !duplicated ? true : false;;
}


bool
set_tunnel_endpoint_port( uint32_t vni, struct in_addr ip_addr, uint16_t port ) {
  assert( tunnel_endpoints != NULL );

  if ( !valid_vni( vni ) ) {
    return false;
  }

  list *l = ( list * ) search_hash( tunnel_endpoints, &vni );
  if ( l == NULL ) {
    return false;
  }

  bool updated = false;
  pthread_mutex_lock( &l->mutex );
  for ( list_element *e = l->head; e != NULL; e = e->next ) {
    if ( e->data != NULL ) {
      tunnel_endpoint *t = e->data;
      if ( t->ip_addr.s_addr == ip_addr.s_addr ) {
        t->port = htons( port );
        updated = true;
        break;
      }
    }
  }
  pthread_mutex_unlock( &l->mutex );

  return updated;
}


bool
delete_tunnel_endpoint( uint32_t vni, struct in_addr ip_addr ) {
  assert( tunnel_endpoints != NULL );

  if ( !valid_vni( vni ) ) {
    return false;
  }

  tunnel_endpoint *delete = NULL;
  list *l = ( list * ) search_hash( tunnel_endpoints, &vni );
  if ( l == NULL ) {
    return false;
  }

  for ( list_element *e = l->head; e != NULL; e = e->next ) {
    if ( e->data == NULL ) {
      continue;
    }
    tunnel_endpoint *tep = e->data;
    if ( tep->vni == vni && tep->ip_addr.s_addr == ip_addr.s_addr ) {
      delete = tep;
      break;
    }
  }

  if ( delete == NULL ) {
    return false;
  }

  delete_element( l, delete );
  free( delete );

  if ( l->head == NULL ) {
    void *deleted = delete_hash( tunnel_endpoints, &vni );
    if ( deleted != NULL ) {
      delete_list( deleted );
    }
  }

  return true;
}


static uint32_t
get_vni_value( struct vxlanhdr *vxlan ) {
  uint32_t vni = 0;

  vni |= ( uint32_t ) ( vxlan->vni[ 0 ] << 16 );
  vni |= ( uint32_t ) ( vxlan->vni[ 1 ] << 8 );
  vni |= ( uint32_t ) vxlan->vni[ 2 ];

  return vni;
}


static bool
distribute_packet( ethdev *dev, packet_buffer *packet ) {
  assert( dev != NULL );
  assert( packet != NULL );
  assert( packet->data != NULL );
  assert( packet->ip != NULL );
  assert( packet->udp != NULL );
  assert( packet->vxlan != NULL );

  struct iphdr *ip = packet->ip;
  struct udphdr *udp = packet->udp;
  struct vxlanhdr *vxlan = packet->vxlan;
  uint32_t vni = get_vni_value( vxlan );

  list *destination_list = lookup_tunnel_endpoints( vni );
  if ( destination_list == NULL ) {
    return true;
  }

  udp->check = 0;

  struct sockaddr_in dst;
  memset( &dst, 0, sizeof( dst ) );
  dst.sin_family = AF_INET;
  dst.sin_port = IPPROTO_UDP;

  pthread_mutex_lock( &destination_list->mutex );
  for ( list_element *e = destination_list->head; e != NULL; e = e->next ) {
    tunnel_endpoint *tep = e->data;

    if ( tep->ip_addr.s_addr == ip->saddr ) {
      continue;
    }

    if ( tep->port > 0 ) {
      udp->dest = tep->port;
    }
    ip->daddr = tep->ip_addr.s_addr;
    ip->check = 0;

    dst.sin_addr = tep->ip_addr;

    while ( running ) {
      fd_set fds;
      FD_ZERO( &fds );
      FD_SET( dev->fd, &fds );
      struct timespec timeout = { 0, 0 };
      int ret = pselect( dev->fd + 1, NULL, &fds, NULL, &timeout, NULL );
      if ( ret < 0 ) {
        if ( errno == EINTR ) {
          continue;
        }
        char buf[ 256 ];
        char *error_string = safe_strerror_r( errno, buf, sizeof( buf ) );
        error( "Failed to select ( errno = %s [%d] ).", error_string, errno );
        goto error;
      }
      if ( !FD_ISSET( dev->fd, &fds ) ) {
        continue;
      }
      int err = 0;
      ssize_t length = send_to_ethdev( dev, packet->data, packet->length, &dst, &err );
      if ( length < 0 ) {
        if ( err != ECONNRESET && err != EMSGSIZE && err != ENOBUFS && err != EHOSTUNREACH &&
             err != ENETDOWN && err != ENETUNREACH ) {
          error( "Failed to send a packet to %s ( data = %p, length = %u ).",
                 dev->name, packet->data, packet->length );
          goto error;
        }
      }
      break;
    }
    if ( !running ) {
      break;
    }
    tep->counters.packet++;
    tep->counters.octet += packet->length;
  }
  pthread_mutex_unlock( &destination_list->mutex );

  return true;

error:
  pthread_mutex_unlock( &destination_list->mutex );

  return false;
}


static void
wait_for_new_packets() {
  pthread_mutex_lock( &mutex );

  struct timespec timeout;
  clock_gettime( CLOCK_REALTIME, &timeout );
  timeout.tv_sec += 1;
  pthread_cond_timedwait( &cond, &mutex, &timeout );

  pthread_mutex_unlock( &mutex );
}


void *
distributor_main( void *args ) {
  assert( args != NULL );
  assert( received_packets != NULL );
  assert( free_packet_buffers != NULL );

  ethdev *dev = args;

  info( "Distributer thread is started ( pid = %u, tid = %u ).",
        getpid(), distributor_thread );

  create_tunnel_endpoints();

  bool err = false;
  while ( running ) {
    wait_for_new_packets();

    packet_buffer *packet = NULL;
    while ( ( packet = peek( received_packets ) ) != NULL ) {
      if ( packet->length == 0 ) {
        continue;
      }

      bool ret = distribute_packet( dev, packet );
      if ( !ret ) {
        err = true;
        break;
      }

      dequeue( received_packets );
      enqueue( free_packet_buffers, packet );
    }

    if ( err ) {
      break;
    }
  }
 
  running = false;

  delete_tunnel_endpoints();

  if ( !err ) {
    info( "Distributer thread is terminated ( pid = %u, tid = %u ).",
          getpid(), distributor_thread );
  }
  else {
    critical( "Distributer thread is terminated due to an error ( pid = %u, tid = %u ).",
              getpid(), distributor_thread );
  }

  return NULL;
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
