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
#include <fcntl.h>
#include <inttypes.h>
#include <linux/limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>
#include "reflector_ctrl_client.h"
#include "log.h"
#include "reflector_common.h"
#include "wrapper.h"


static int fd = -1;


static ssize_t
send_request( void *reply, size_t *length ) {
  assert( fd > 0 );
  assert( reply != NULL );
  assert( length != NULL );
  assert( *length > 0 );

  struct sockaddr_un saddr;
  memset( &saddr, 0, sizeof( saddr ) );
  saddr.sun_family = AF_UNIX;
  memset( saddr.sun_path, '\0', sizeof( saddr.sun_path ) );
  strncpy( saddr.sun_path, CTRL_SERVER_SOCK_FILE, sizeof( saddr.sun_path ) - 1 );
  int ret = connect( fd, ( const struct sockaddr * ) &saddr, ( socklen_t ) sizeof( saddr ) );
  if ( ret < 0 ) {
    char buf[ 256 ];
    char *error_string = safe_strerror_r( errno, buf, sizeof( buf ) );
    error( "Failed to connect ( ret = %d, errno = %s [%d] ).", ret, error_string, errno );
    return -1;
  }

  return send_command( fd, reply, length );
}


static void
print_dump_tunnel_endpoint_header() {
  printf( "   VNI    |   IP address    | UDP port | Packet count | Octet count\n" );
  printf( "----------+-----------------+----------+--------------+--------------\n" );
}


static void
dump_tunnel_endpoint( tunnel_endpoint *tep ) {
  assert( tep != NULL );

  char ip_addr[ INET_ADDRSTRLEN ];

  const char *ip_string = inet_ntop( AF_INET, ( const void * ) &tep->ip_addr, ip_addr, sizeof( ip_addr ) );

  if ( ntohs( tep->port ) > 0 ) {
    printf( " %#8x | %15s | %8u | %12" PRIu64 " | %12" PRIu64 "\n",
            tep->vni, ip_string, ntohs( tep->port ), tep->counters.packet, tep->counters.octet );
  }
  else {
    printf( " %#8x | %15s | %8s | %12" PRIu64 " | %12" PRIu64 "\n",
            tep->vni, ip_string, "-", tep->counters.packet, tep->counters.octet );
  }
}


static bool
handle_reply( void *reply, size_t length, uint8_t *reason ) {
  assert( reply != NULL );
  assert( length >= sizeof( command_reply_header ) );
  assert( reason != NULL );

  command_reply_header *header = reply;
  uint8_t status = header->status;
  bool ret = false;
  switch ( status ) {
    case STATUS_OK:
      *reason = SUCCEEDED;
      ret = true;
      break;

    case STATUS_NG:
      *reason = header->reason;
      ret = false;
      break;

    default:
      error( "Undefined command status ( %#x ).", status );
      ret = false;
      break;
  }

  if ( !ret ) {
    return false;
  }

  uint8_t type = header->type;
  switch ( type ) {
    case LIST_TEP_REPLY:
    {
      unsigned int count = ( unsigned int ) ( header->length - offsetof( list_tep_reply, tep ) ) / sizeof( tunnel_endpoint );
      tunnel_endpoint *tep = ( ( list_tep_reply * ) reply )->tep;
      for ( unsigned int i = 0; i < count; i++ ) {
        dump_tunnel_endpoint( tep );
        tep++;
      }
    }
    break;

    default:
      break;
  }

  return true;
}


static bool
recv_reply( uint32_t xid, uint8_t *reason ) {
  assert( reason != NULL );

  bool ret = true;
  uint8_t flags = FLAG_MORE;
  int n_replies = 0;

  while ( flags & FLAG_MORE ) {
    char reply[ COMMAND_MESSAGE_LENGTH ];
    memset( reply, 0, sizeof( reply ) );
    size_t length = COMMAND_MESSAGE_LENGTH;

    ssize_t retval = recv_command( fd, reply, &length );
    if ( retval < sizeof( command_reply_header ) ) {
      *reason = OTHER_ERROR;
      ret = false;
      break;
    }
    command_reply_header *header = ( command_reply_header * ) reply;
    if ( xid != header->xid ) {
      *reason = OTHER_ERROR;
      ret = false;
      break;
    }

    switch ( header->type ) {
      case LIST_TEP_REPLY:
        if ( n_replies == 0 && header->status == STATUS_OK ) {
          print_dump_tunnel_endpoint_header();
        }
        break;

      default:
        break;
    }

    ret &= handle_reply( ( void * ) reply, length, reason );
    flags = header->flags;
    n_replies++;
  }

  return ret;
}


bool
add_tep( uint32_t vni, struct in_addr ip_addr, uint16_t port, uint8_t *reason ) {
  assert( fd >= 0 );
  assert( reason != NULL );

  add_tep_request request;
  memset( &request, 0, sizeof( add_tep_request ) );
  request.header.xid = ( uint32_t ) rand();
  request.header.type = ADD_TEP_REQUEST;
  request.header.length = ( uint32_t ) sizeof( add_tep_request );
  request.vni = vni;
  request.ip_addr = ip_addr;
  request.port = port;
  size_t length = sizeof( add_tep_request );

  ssize_t ret = send_request( ( void * ) &request, &length );
  if ( ret < 0 ) {
    *reason = OTHER_ERROR;
    return false;
  }

  return recv_reply( request.header.xid, reason );
}


bool
set_tep( uint32_t vni, struct in_addr ip_addr, uint16_t set_bitmap, uint16_t port, uint8_t *reason ) {
  assert( fd >= 0 );
  assert( reason != NULL );

  set_tep_request request;
  memset( &request, 0, sizeof(set_tep_request ) );
  request.header.xid = ( uint32_t ) rand();
  request.header.type = SET_TEP_REQUEST;
  request.header.length = ( uint32_t ) sizeof( set_tep_request );
  request.vni = vni;
  request.ip_addr = ip_addr;
  request.port = port;
  request.set_bitmap = set_bitmap;
  size_t length = sizeof( set_tep_request );

  ssize_t ret = send_request( ( void * ) &request, &length );
  if ( ret < 0 ) {
    *reason = OTHER_ERROR;
    return false;
  }

  return recv_reply( request.header.xid, reason );
}


bool
delete_tep( uint32_t vni, struct in_addr ip_addr, uint8_t *reason ) {
  assert( fd >= 0 );
  assert( reason != NULL );

  del_tep_request request;
  memset( &request, 0, sizeof( del_tep_request ) );
  request.header.xid = ( uint32_t ) rand();
  request.header.type = DEL_TEP_REQUEST;
  request.header.length = ( uint32_t ) sizeof( del_tep_request );
  request.vni = vni;
  request.ip_addr = ip_addr;
  size_t length = sizeof( del_tep_request );

  ssize_t ret = send_request( ( void * ) &request, &length );
  if ( ret < 0 ) {
    *reason = OTHER_ERROR;
    return false;
  }

  return recv_reply( request.header.xid, reason );
}


bool
list_tep( uint32_t vni, uint8_t *reason ) {
  assert( fd >= 0 );
  assert( reason != NULL );

  list_tep_request request;
  memset( &request, 0, sizeof( list_tep_request ) );
  request.header.xid = ( uint32_t ) rand();
  request.header.type = LIST_TEP_REQUEST;
  request.header.length = ( uint32_t ) sizeof( list_tep_request );
  request.vni = vni;
  size_t length = sizeof( list_tep_request );

  ssize_t ret = send_request( ( void * ) &request, &length );
  if ( ret < 0 ) {
    *reason = OTHER_ERROR;
    return false;
  }

  return recv_reply( request.header.xid, reason );
}


bool
init_reflector_ctrl_client() {
  assert( fd < 0 );

  return init_ctrl_client( &fd );
}


bool
finalize_reflector_ctrl_client() {
  assert( fd >= 0 );

  return finalize_ctrl_client( fd );
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
