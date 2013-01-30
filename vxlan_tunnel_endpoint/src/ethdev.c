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


#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <linux/if_ether.h>
#include <net/if.h>
#include <netpacket/packet.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include "checks.h"
#include "ethdev.h"
#include "log.h"
#include "wrapper.h"


#define ETHDEV_TXQ_LEN 10000


bool
init_ethdev( const char *name, uint16_t port, ethdev **dev ) {
  assert( name != NULL );
  assert( port > 0 );
  assert( dev != NULL );

  *dev = malloc( sizeof( ethdev ) );
  assert( dev != NULL );

  memset( ( *dev )->name, '\0', sizeof( ( *dev )->name ) );
  strncpy( ( *dev )->name, name, sizeof( ( *dev )->name ) - 1 );

  int fd = -1;
  int dummy_fd = -1;
  char buf[ 256 ];

  fd = socket( AF_INET, SOCK_RAW, IPPROTO_UDP );
  if ( fd < 0 ) {
    char *error_string = safe_strerror_r( errno, buf, sizeof( buf ) );
    error( "Failed to open a socket ( fd = %d, errno = %s [%d] ).", fd, error_string, errno );
    goto error;
  }
  ( *dev )->fd = fd;

  struct ifreq ifr;
  memset( &ifr, 0, sizeof( ifr ) );
  strncpy( ifr.ifr_name, name, IFNAMSIZ );
  int ret = ioctl( fd, SIOCGIFINDEX, ( void * ) &ifr );
  if ( ret != 0 ) {
    char *error_string = safe_strerror_r( errno, buf, sizeof( buf ) );
    error( "Failed to retrieve interface index ( fd = %d, ret = %d, errno = %s [%d] ).",
           fd, ret, error_string, errno );
    goto error;
  }
  ( *dev )->ifindex = ifr.ifr_ifindex;

  ifr.ifr_flags = 0;
  ret = ioctl( fd, SIOCGIFFLAGS, ( void * ) &ifr );
  if ( ret < 0 ) {
    char *error_string = safe_strerror_r( errno, buf, sizeof( buf ) );
    error( "Failed to retrieve interface flags ( fd = %d, ret = %d, errno = %s [%d] ).",
           fd, ret, error_string, errno );
    goto error;
  }

  ifr.ifr_flags |= IFF_UP | IFF_RUNNING;
  ret = ioctl( fd, SIOCSIFFLAGS, ( void * ) &ifr );
  if ( ret < 0 ) {
    char *error_string = safe_strerror_r( errno, buf, sizeof( buf ) );
    error( "Failed to set interface flags to %#x ( fd = %d, ret = %d, errno = %s [%d] ).",
           ifr.ifr_flags, fd, ret, error_string, errno );
    goto error;
  }

  ifr.ifr_qlen = ETHDEV_TXQ_LEN;
  ret = ioctl( fd, SIOCSIFTXQLEN, ( void * ) &ifr );
  if ( ret < 0 ) {
    char *error_string = safe_strerror_r( errno, buf, sizeof( buf ) );
    error( "Failed to set TX queue length to %u ( fd = %d, ret = %d, errno = %s [%d] ).",
           ifr.ifr_qlen, fd, ret, error_string, errno );
    goto error;
  }

  ret = setsockopt( fd, SOL_SOCKET, SO_BINDTODEVICE, &ifr, sizeof( ifr ) );
  if ( ret < 0 ) {
    char *error_string = safe_strerror_r( errno, buf, sizeof( buf ) );
    error( "Failed to set SO_BINDTODEVICE option ( fd = %d, ret = %d, errno = %s [%d] ).",
           fd, ret, error_string, errno );
    goto error;
  }

  int value = 1;
  ret = setsockopt( fd, IPPROTO_IP, IP_HDRINCL, &value, sizeof( value ) );
  if ( ret < 0 ) {
    char *error_string = safe_strerror_r( errno, buf, sizeof( buf ) );
    error( "Failed to set IP_HDRINCL option ( fd = %d, ret = %d, errno = %s [%d] ).",
           fd, ret, error_string, errno );
    goto error;
  }

  struct sockaddr_in sin;
  memset( &sin, 0, sizeof( sin ) );
  sin.sin_family = AF_INET;
  sin.sin_port = IPPROTO_UDP;
  sin.sin_addr.s_addr = htonl( INADDR_ANY );

  ret = bind( fd, ( struct sockaddr * ) &sin, sizeof( sin ) );
  if ( ret < 0 ) {
    char *error_string = safe_strerror_r( errno, buf, sizeof( buf ) );
    error( "Failed to bind ( fd = %d, ret = %d, errno = %s [%d] ).",
           fd, ret, error_string, errno );
    goto error;
  }

  // Create a dummy socket not to send ICMP destination unreachable messages
  dummy_fd = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP );
  if ( dummy_fd < 0 ) {
    char *error_string = safe_strerror_r( errno, buf, sizeof( buf ) );
    error( "Failed to open a socket ( dummy_fd = %d, errno = %s [%d] ).",
           dummy_fd, error_string, errno );
    goto error;
  }
  ( *dev )->dummy_fd = dummy_fd;

  value = 0;
  ret = setsockopt( dummy_fd, SOL_SOCKET, SO_RCVBUF, &value, sizeof( value ) );
  if ( ret < 0 ) {
    char *error_string = safe_strerror_r( errno, buf, sizeof( buf ) );
    error( "Failed to set SO_RCVBUF option ( dummy_fd = %d, ret = %d, errno = %s [%d] ).",
           dummy_fd, ret, error_string, errno );
    goto error;
  }

  memset( &sin, 0, sizeof( sin ) );
  sin.sin_family = AF_INET;
  sin.sin_addr.s_addr = htonl( INADDR_ANY );
  sin.sin_port = htons( port );

  ret = bind( dummy_fd, ( struct sockaddr * ) &sin, sizeof( sin ) );
  if ( ret < 0 ) {
    char *error_string = safe_strerror_r( errno, buf, sizeof( buf ) );
    error( "Failed to bind ( dummy_fd = %d, ret = %d, errno = %s [%d] ).",
           dummy_fd, ret, error_string, errno );
    goto error;
  }

  return true;

error:
  if ( *dev != NULL ) {
    free( *dev );
    *dev = NULL;
  }
  if ( fd >= 0 ) {
    close( fd );
  }
  if ( dummy_fd >= 0 ) {
    close( dummy_fd );
  }
  return false;
}


bool
close_ethdev( ethdev *dev ) {
  assert( dev != NULL );

  if ( dev->fd < 0 ) {
    warn( "%s is already closed ( fd = %d ).", dev->name, dev->fd );
    return false;
  }
  if ( dev->dummy_fd < 0 ) {
    warn( "%s is already closed ( dummy_fd = %d ).", dev->name, dev->dummy_fd );
    return false;
  }

  char buf[ 256 ];

  int ret = close( dev->fd );
  if ( ret < 0 ) {
    char *error_string = safe_strerror_r( errno, buf, sizeof( buf ) );
    error( "Failed to close a socket ( fd = %d, ret = %d, errno = %s [%d] ).",
           dev->fd, ret, error_string, errno );
    return false;
  }

  ret = close( dev->dummy_fd );
  if ( ret < 0 ) {
    char *error_string = safe_strerror_r( errno, buf, sizeof( buf ) );
    error( "Failed to close a socket ( dummy_fd = %d, ret = %d, errno = %s [%d] ).",
           dev->dummy_fd, ret, error_string, errno );
    return false;
  }

  free( dev );

  return true;
}


ssize_t
recv_from_ethdev( ethdev *dev, char *data, size_t length, int *err ) {
  assert( dev != NULL );
  assert( dev->fd >= 0 );
  assert( data != NULL );
  assert( length > 0 );

  ssize_t ret = recv( dev->fd, data, length, 0 );
  if ( ret < 0 ) {
    if ( errno == EAGAIN || errno == EINTR ) {
      return 0;
    }
    if ( err != NULL ) {
      *err = errno;
    }
    char buf[ 256 ];
    char *error_string = safe_strerror_r( errno, buf, sizeof( buf ) );
    error( "Failed to receive a packet ( ret = %d, errno = %s [%d] ).", ret, error_string, errno );
    return -1;
  }

  return ret;
}


ssize_t
send_to_ethdev( ethdev *dev, const char *data, size_t length, struct sockaddr_in *dst, int *err ) {
  assert( dev != NULL );
  assert( dev->fd >= 0 );
  assert( data != NULL );
  assert( length > 0 );
  assert( dst != NULL );

  ssize_t ret = sendto( dev->fd, data, length, 0, ( struct sockaddr * ) dst, sizeof( struct sockaddr_in ) );
  if ( ret < 0 ) {
    if ( err != NULL ) {
      *err = errno;
    }
    if ( errno == EAGAIN || errno == EINTR ) {
      return 0;
    }
    char buf[ 256 ];
    char *error_string = safe_strerror_r( errno, buf, sizeof( buf ) );
    error( "Failed to send a packet ( ret = %d, errno = %s [%d] ).", ret, error_string, errno );
    return -1;
  }

  return ret;
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
