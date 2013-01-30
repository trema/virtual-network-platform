/*
 * Copyright (C) 2012 upa@haeena.net
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
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <linux/if_ether.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <pthread.h>
#include "hash.h"
#include "log.h"
#include "net.h"
#include "fdb.h"
#include "wrapper.h"


struct multicast_group_table {
  struct hash groups;
  pthread_mutex_t mutex;
};


static struct vxlan *vxlan = NULL;
static struct multicast_group_table *multicast_groups = NULL;


static bool
init_multicast_group_table() {
  assert( multicast_groups == NULL );

  multicast_groups = malloc( sizeof( struct multicast_group_table ) );
  assert( multicast_groups != NULL );
  memset( multicast_groups, 0, sizeof( struct multicast_group_table ) );

  init_hash( &multicast_groups->groups, sizeof( struct in_addr ) );
  pthread_mutexattr_t attr;
  pthread_mutexattr_init( &attr );
  pthread_mutexattr_settype( &attr, PTHREAD_MUTEX_RECURSIVE_NP );
  pthread_mutex_init( &multicast_groups->mutex, &attr );

  return true;
}


static bool
finalize_multicast_group_table() {
  assert( multicast_groups != NULL );

  destroy_hash( &multicast_groups->groups );
  free( multicast_groups );
  multicast_groups = NULL;

  return true;
}


static void
lock_multicast_group_table() {
  assert( multicast_groups != NULL );

  pthread_mutex_lock( &multicast_groups->mutex );
}


static void
unlock_multicast_group_table() {
  assert( multicast_groups != NULL );

  pthread_mutex_unlock( &multicast_groups->mutex );
}


static bool
add_user_into_multicast_group( struct in_addr maddr, unsigned int *n_users ) {
  assert( multicast_groups != NULL );
  assert( n_users != NULL );

  if ( !IN_MULTICAST( ntohl( maddr.s_addr ) ) ) {
    return false;
  }

  lock_multicast_group_table();

  unsigned int *in_use = search_hash( &multicast_groups->groups, &maddr );
  if ( in_use == NULL ) {
    in_use = malloc( sizeof( unsigned int ) );
    assert( in_use != NULL );
    *in_use = 0;
    insert_hash( &multicast_groups->groups, in_use, &maddr );
  }
  ( *in_use )++;

  *n_users = *in_use;

  unlock_multicast_group_table();

  return true;
}


static bool
delete_user_from_multicast_group( struct in_addr maddr, unsigned int *n_users ) {
  assert( multicast_groups != NULL );
  assert( n_users != NULL );

  if ( !IN_MULTICAST( ntohl( maddr.s_addr ) ) ) {
    return false;
  }

  lock_multicast_group_table();

  unsigned int *in_use = search_hash( &multicast_groups->groups, &maddr );
  if ( in_use == NULL ) {
    *n_users = 0;
    unlock_multicast_group_table();
    return false;
  }

  ( *in_use )--;
  *n_users = *in_use;
  if ( *in_use == 0 ) {
    in_use = delete_hash( &multicast_groups->groups, &maddr );
    if ( in_use != NULL ) {
      free( in_use );
      in_use = NULL;
    }
  }

  unlock_multicast_group_table();

  return true;
}


void
send_etherframe_from_vxlan_to_local( struct vxlan_instance *instance,
                                     struct ether_header *ether, size_t len ) {
  assert( vxlan != NULL );
  assert( instance != NULL );
  assert( ether != NULL );
  assert( len > 0 );

  if ( !instance->activated || !vxlan->active ) {
    return;
  }

  ssize_t ret = write( instance->tap_sock, ether, len );
  if ( ret != ( ssize_t ) len ) {
    char buf[ 256 ];
    char *error_string = safe_strerror_r( errno, buf, sizeof( buf ) );
    warn( "Failed to write an Ethernet frame to a tap interface ( socket = %d, "
          "len = %u, ret = %d, errno = %s [%d] ).",
          instance->tap_sock, len, ret, error_string, errno );
  }
}


void
send_etherframe_from_local_to_vxlan( struct vxlan_instance *instance,
                                     struct ether_header *ether, size_t len ) {
  assert( vxlan != NULL );
  assert( instance != NULL );
  assert( ether != NULL );
  assert( len > 0 );

  if ( !instance->activated || !vxlan->active ) {
    return;
  }

  struct vxlanhdr vhdr;
  memset( &vhdr, 0, sizeof( vhdr ) );
  vhdr.flags = VXLAN_VALIDFLAG;
  memcpy( vhdr.vni, instance->vni, VXLAN_VNISIZE );

  struct iovec iov[ 2 ];
  memset( iov, 0, sizeof( iov ) );
  iov[ 0 ].iov_base = &vhdr;
  iov[ 0 ].iov_len  = sizeof( vhdr );
  iov[ 1 ].iov_base = ether;
  iov[ 1 ].iov_len  = len;

  struct msghdr mhdr;
  memset( &mhdr, 0, sizeof( mhdr ) );
  mhdr.msg_iov = iov;
  mhdr.msg_iovlen = 2;
  mhdr.msg_controllen = 0;

  char buf[ 256 ];

  struct fdb_entry *entry = fdb_search_entry( instance->fdb, ether->ether_dhost );
  if ( entry == NULL ) {
    mhdr.msg_name = &instance->addr;
    mhdr.msg_namelen = sizeof( instance->addr );
    if ( sendmsg( instance->udp_sock, &mhdr, 0 ) < 0 ) {
      char *error_string = safe_strerror_r( errno, buf, sizeof( buf ) );
      warn( "Failed to send a vxlan message ( errno = %s [%d] ).", error_string, errno );
    }
  }
  else {
    entry->vtep_addr.sin_port = htons( instance->port );
    mhdr.msg_name = &entry->vtep_addr;
    mhdr.msg_namelen = sizeof( entry->vtep_addr );
    if ( sendmsg( instance->udp_sock, &mhdr, 0 ) < 0 ) {
      char *error_string = safe_strerror_r( errno, buf, sizeof( buf ) );
      warn( "Failed to send a vxlan message ( errno = %s [%d] ).", error_string, errno );
    }
  }
}


static bool
getifaddr( char *dev, struct in_addr *addr ) {
  assert( dev != NULL );
  assert( addr != NULL );

  int fd = socket( AF_INET, SOCK_DGRAM, 0 );

  struct ifreq ifr;
  memset( &ifr, 0, sizeof( ifr ) );
  strncpy( ifr.ifr_name, dev, sizeof( ifr.ifr_name ) - 1 );

  if ( ioctl( fd, SIOCGIFADDR, &ifr ) < 0 ) {
    close( fd );
    return false;
  }

  close( fd );

  struct sockaddr_in *saddr = ( struct sockaddr_in * ) &( ifr.ifr_addr );
  *addr = saddr->sin_addr;

  return true;
}


static bool
set_ipv4_multicast_join_and_iface( int socket, struct in_addr maddr, char *ifname ) {
  assert( ifname != NULL );

  if ( !IN_MULTICAST( ntohl( maddr.s_addr ) ) ) {
    return false;
  }

  unsigned int n_users = 0;
  add_user_into_multicast_group( maddr, &n_users );
  if ( n_users > 1 ) {
    // Already joined.
    return true;
  }
  else if ( n_users != 1 ) {
    return false;
  }

  struct ip_mreq mreq;
  memset( &mreq, 0, sizeof( mreq ) );
  mreq.imr_multiaddr = maddr;
  if ( !getifaddr( ifname, &mreq.imr_interface ) ) {
    return false;
  }

  char buf[ 256 ];

  int ret = setsockopt( socket, IPPROTO_IP, IP_ADD_MEMBERSHIP,
                        ( char * ) &mreq, sizeof( mreq ) );
  if ( ret < 0 ) {
    char *error_string = safe_strerror_r( errno, buf, sizeof( buf ) );
    error( "Failed to send a membership report ( socket = %d, ret = %d, errno = %s [%d] ).",
           socket, ret, error_string, errno );
    return false;
  }

  ret = setsockopt( socket, IPPROTO_IP, IP_MULTICAST_IF,
                    ( char * ) &mreq.imr_interface, sizeof( mreq.imr_interface ) );
  if ( ret < 0 ) {
    char *error_string = safe_strerror_r( errno, buf, sizeof( buf ) );
    error( "Failed to set a network interface ( socket = %d, ret = %d, errno = %s [%d] ).",
           socket, ret, error_string, errno );
    return false;
  }

  return true;
}


static bool
set_ipv4_multicast_leave( int socket, struct in_addr maddr, char *ifname ) {
  assert( ifname != NULL );

  if ( !IN_MULTICAST( ntohl( maddr.s_addr ) ) ) {
    return false;
  }

  unsigned int n_users = 0;
  delete_user_from_multicast_group( maddr, &n_users );
  if ( n_users > 0 ) {
    // Still used.
    return true;
  }

  struct ip_mreq mreq;
  memset( &mreq, 0, sizeof( mreq ) );
  mreq.imr_multiaddr = maddr;
  if ( !getifaddr( ifname, &mreq.imr_interface ) ) {
    return false;
  }

  int ret = setsockopt( socket, IPPROTO_IP, IP_DROP_MEMBERSHIP,
                        ( char * ) &mreq, sizeof( mreq ) );
  if ( ret < 0 ) {
    char buf[ 256 ];
    char *error_string = safe_strerror_r( errno, buf, sizeof( buf ) );
    error( "Failed to send a leave group ( socket = %d, ret = %d, errno = %s [%d] ).",
           socket, ret, error_string, errno );
    return false;
  }

  return true;
}


static bool
set_ipv4_multicast_loop( int socket, int stat ) {
  assert( socket >= 0 );

  int ret = setsockopt( socket, IPPROTO_IP, IP_MULTICAST_LOOP,
                        ( char * ) &stat, sizeof( stat ) );
  if ( ret < 0 ) {
    char buf[ 256 ];
    char *error_string = safe_strerror_r( errno, buf, sizeof( buf ) );
    error( "Failed to turn off multicast loopback ( socket = %d, ret = %d, errno = %s [%d] ).",
           socket, ret, error_string, errno );
    return false;
  }

  return true;
}


static bool
set_ipv4_multicast_ttl( int socket, int ttl ) {
  assert( socket >= 0 );

  int ret = setsockopt( socket, IPPROTO_IP, IP_MULTICAST_TTL,
                        ( char * ) &ttl, sizeof( ttl ) );
  if ( ret < 0 ) {
    char buf[ 256 ];
    char *error_string = safe_strerror_r( errno, buf, sizeof( buf ) );
    error( "Failed to set TTL value to %d ( socket = %d, ret = %d, errno = %s [%d] ).",
           ttl, socket, ret, error_string, errno );
    return false;
  }

  return true;
}


static bool
bind_ipv4_inaddrany( int socket, uint16_t port ) {
  assert( socket >= 0 );
  assert( port > 0 );

  struct sockaddr_in saddr_in;

  memset( &saddr_in, 0, sizeof( saddr_in ) );
  saddr_in.sin_family = AF_INET;
  saddr_in.sin_port = htons( port );
  saddr_in.sin_addr.s_addr = INADDR_ANY;

  int ret = bind( socket, ( struct sockaddr * ) &saddr_in, sizeof( saddr_in ) );
  if ( ret < 0 ) {
    char buf[ 256 ];
    char *error_string = safe_strerror_r( errno, buf, sizeof( buf ) );
    error( "Failed to bind a socket ( fd = %d, errno = %s [%d] ).",
           socket, error_string, errno );
    return false;
  }

  return true;
}


bool
ipv4_multicast_join( struct in_addr addr ) {
  assert( vxlan != NULL );

  if ( !IN_MULTICAST( ntohl( addr.s_addr ) ) ) {
    return false;
  }

  update_interface_state();
  if ( !vxlan->active ) {
    return false;
  }

  return set_ipv4_multicast_join_and_iface( vxlan->udp_sock, addr, vxlan->ifname );
}


bool
ipv4_multicast_leave( struct in_addr addr ) {
  assert( vxlan != NULL );

  if ( !IN_MULTICAST( ntohl( addr.s_addr ) ) ) {
    return false;
  }

  update_interface_state();
  if ( !vxlan->active ) {
    return false;
  }

  return set_ipv4_multicast_leave( vxlan->udp_sock, addr, vxlan->ifname );
}


bool
update_interface_state() {
  assert( vxlan != NULL );

  struct in_addr addr;
  if ( getifaddr( vxlan->ifname, &addr ) ) {
    vxlan->active = true;
  }
  else {
    vxlan->active = false;
  }

  return true;
}


bool
init_net( struct vxlan *_vxlan ) {
  assert( _vxlan != NULL );

  vxlan = _vxlan;

  vxlan->udp_sock = socket( AF_INET, SOCK_DGRAM, 0 );
  if ( vxlan->udp_sock < 0 ) {
    char buf[ 256 ];
    char *error_string = safe_strerror_r( errno, buf, sizeof( buf ) );
    error( "Failed to create a socket for IPv4 ( ret = %d, errno = %s [%d] ).",
           vxlan->udp_sock, error_string, errno );
    goto error;
  }

  bool ret = bind_ipv4_inaddrany( vxlan->udp_sock, vxlan->port );
  if ( !ret ) {
    goto error;
  }
  ret = set_ipv4_multicast_loop( vxlan->udp_sock, 0 );
  if ( !ret ) {
    goto error;
  }
  ret = set_ipv4_multicast_ttl( vxlan->udp_sock, VXLAN_MCAST_TTL );
  if ( !ret ) {
    goto error;
  }

  ret = update_interface_state();
  if ( !ret ) {
    goto error;
  }

  ret = init_multicast_group_table();
  if ( !ret ) {
    goto error;
  }

  return true;

error:
  if ( vxlan->udp_sock >= 0 ) {
    close( vxlan->udp_sock );
  }

  return false;
}


bool
finalize_net() {
  assert( vxlan != NULL );

  if ( vxlan->udp_sock >= 0 ) {
    close( vxlan->udp_sock );
  }

  return finalize_multicast_group_table();
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
