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


#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <unistd.h>
#include "iftap.h"
#include "log.h"
#include "wrapper.h"


int
tap_alloc( const char *dev ) {
  assert( dev != NULL );

  char buf[ 256 ];

  int fd = open( "/dev/net/tun", O_RDWR );
  if ( fd < 0 ) {
    char *error_string = safe_strerror_r( errno, buf, sizeof( buf ) );
    error( "Failed to create a control channel of the tap interface ( ret = %d, errno = %s [%d] ).",
           fd, error_string, errno );
    return -1;
  }

  struct ifreq ifr;
  memset( &ifr, 0, sizeof( ifr ) );
  ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
  strncpy( ifr.ifr_name, dev, IFNAMSIZ );
  int ret = ioctl( fd, TUNSETIFF, ( void * ) &ifr );
  if ( ret < 0 ) {
    close( fd );
    char *error_string = safe_strerror_r( errno, buf, sizeof( buf ) );
    error( "Failed to create a tap interface ( dev = %s, ret = %d, errno = %s [%d] ).",
           dev, ret, error_string, errno );
    return -1;
  }

  return fd;
}


static bool
get_interface_flags( int fd, struct ifreq *ifr ) {
  assert( fd >= 0 );
  assert( ifr != NULL );

  ifr->ifr_flags = 0;
  int ret = ioctl( fd, SIOCGIFFLAGS, ( void * ) ifr );
  if ( ret < 0 ) {
    char buf[ 256 ];
    char *error_string = safe_strerror_r( errno, buf, sizeof( buf ) );
    error( "Failed to retrieve interface flags ( errno = %s [%d] ).", error_string, errno );
    return false;
  }

  return true;
}


bool
tap_up( const char *dev ) {
  assert( dev != NULL );

  char buf[ 256 ];

  int fd = socket( AF_INET, SOCK_DGRAM, 0 );
  if ( fd < 0 ) {
    char *error_string = safe_strerror_r( errno, buf, sizeof( buf ) );
    error( "Failed to create a control channel of the tap interface ( ret = %d, errno = %s [%d] ).",
           fd, error_string, errno );
    return false;
  }

  struct ifreq ifr;
  memset( &ifr, 0, sizeof( ifr ) );
  strncpy( ifr.ifr_name, dev, IFNAMSIZ );

  int retval = ioctl( fd, SIOCGIFINDEX, ( void * ) &ifr );
  if ( retval != 0 ) {
    char *error_string = safe_strerror_r( errno, buf, sizeof( buf ) );
    error( "Failed to retrieve interface index ( errno = %s [%d] ).", error_string, errno );
    return false;
  }

  bool ret = get_interface_flags( fd, &ifr );
  if ( !ret ) {
    return false;
  }

  ifr.ifr_flags |= IFF_UP | IFF_RUNNING;
  retval = ioctl( fd, SIOCSIFFLAGS, ( void * ) &ifr );
  if ( retval < 0 ) {
    char *error_string = safe_strerror_r( errno, buf, sizeof( buf ) );
    error( "Failed to make a tap interface ( %s ) up ( ret = %d, errno = %s [%d] ).",
           dev, retval, error_string, errno );
    close( fd );
    return false;
  }

  close( fd );

  return true;
}


bool
tap_down( const char *dev ) {
  assert( dev != NULL );

  char buf[ 256 ];

  int fd = socket( AF_INET, SOCK_DGRAM, 0 );
  if ( fd < 0 ) {
    char *error_string = safe_strerror_r( errno, buf, sizeof( buf ) );
    error( "Failed to create a control channel of the tap interface ( ret = %d, errno = %s [%d] ).",
           fd, error_string, errno );
    return false;
  }

  struct ifreq ifr;
  memset( &ifr, 0, sizeof( ifr ) );
  strncpy( ifr.ifr_name, dev, IFNAMSIZ );

  bool ret = get_interface_flags( fd, &ifr );
  if ( !ret ) {
    return false;
  }

  ifr.ifr_flags &= ~IFF_UP;
  int retval = ioctl( fd, SIOCSIFFLAGS, ( void * ) &ifr );
  if ( retval < 0 ) {
    char *error_string = safe_strerror_r( errno, buf, sizeof( buf ) );
    error( "Failed to make a tap interface ( %s ) down ( ret = %d, errno = %s [%d] ).",
           dev, retval, error_string, errno );
    close( fd );
    return false;
  }

  close( fd );

  return true;
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
