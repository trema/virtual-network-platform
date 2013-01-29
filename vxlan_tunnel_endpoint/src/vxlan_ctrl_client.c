/*
 * Copyright (C) 2012-2013 NEC Corporation
 * NEC Confidential
 */


#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <linux/limits.h>
#include <netinet/ether.h>
#include <netinet/in.h>
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
#include "vxlan_instance.h"
#include "vxlan_ctrl_client.h"
#include "log.h"
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
print_dump_vxlan_global_header() {
  printf( "   IF   | UDP port | Flooding address | Flooding port | Aging time \n");
  printf( "--------+----------+------------------+---------------+------------\n");
}

static void
dump_vxlan_global( struct vxlan *vxlan ) {
  assert( vxlan != NULL );

  char buf[ INET_ADDRSTRLEN ];
  memset( buf, '\0', sizeof( buf ) );
  const char *addr = inet_ntop( AF_INET, &vxlan->flooding_addr.s_addr, buf, sizeof( buf ) );

  printf( " %6s | %8u | %16s | %13u | %9u \n",
          vxlan->ifname, vxlan->port,
          addr, vxlan->flooding_port,
          ( int ) vxlan->aging_time );
}

static void
print_dump_vxlan_instance_header() {
  printf( "   VNI    | Flooding address | UDP port | Aging time |  State\n" );
  printf( "----------+------------------+----------+------------+----------\n" );
}


static void
dump_vxlan_instance( struct vxlan_instance *instance ) {
  assert( instance != NULL );

  uint32_t vni = 0;
  vni = ( uint32_t ) instance->vni[ 2 ];
  vni |= ( uint32_t ) ( instance->vni[ 1 ] << 8 );
  vni |= ( uint32_t ) ( instance->vni[ 0 ] << 16 );

  char buf[ INET_ADDRSTRLEN ];
  memset( buf, '\0', sizeof( buf ) );
  const char *addr = inet_ntop( AF_INET, &instance->addr.sin_addr, buf, sizeof( buf ) );

  printf( " %#8x | %16s | %8u | %10u | %8s\n",
          vni, addr, instance->port, ( int ) instance->aging_time,
          instance->activated ? "Active" : "Inactive" );
}


static void
print_dump_fdb_entry_header() {
  printf( "    MAC address    |   IP address    |  Type   |    Active in     | Expire in\n" );
  printf( "-------------------+-----------------+---------+------------------+-----------\n" );
}


static void
dump_fdb_entry( struct fdb_entry *entry ) {
  assert( entry != NULL );

  char addr[ INET_ADDRSTRLEN ];
  memset( addr, '\0', sizeof( addr ) );
  struct sockaddr_in *saddr = &entry->vtep_addr;
  inet_ntop( AF_INET, ( const void * ) &saddr->sin_addr, addr, sizeof( addr ) );

  struct timespec now = { 0, 0 };
  int ret = clock_gettime( CLOCK_MONOTONIC, &now );
  if ( ret < 0 ) {
    error( "Failed to retrieve monotonic time." );
    return;
  }
  time_t diff = now.tv_sec - entry->created_at.tv_sec;
  char total_time[ 17 ];
  memset( total_time, '\0', sizeof( total_time ) );
  time_t week = diff / 604800;
  diff -= 604800 * week;
  time_t day = diff / 86400;
  diff -= 86400 * day;
  time_t hour = diff / 3600;
  diff -= 3600 * hour;
  time_t minute = diff / 60;
  diff -= 60 * minute;
  time_t second = diff;
  snprintf( total_time, sizeof( total_time ) - 1, "%dw%dd%dh%dm%ds",
            ( int ) week, ( int ) day, ( int ) hour, ( int ) minute, ( int ) second );

  printf( " %02x:%02x:%02x:%02x:%02x:%02x | %15s | %7s | %16s | %8ds\n",
          entry->mac[ 0 ], entry->mac[ 1 ], entry->mac[ 2 ], entry->mac[ 3 ], entry->mac[ 4 ], entry->mac[ 5 ],
          addr, entry->type == FDB_ENTRY_TYPE_DYNAMIC ? "Dynamic" : "Static", total_time,
          entry->ttl > 0 ? ( int ) entry->ttl : 0 );
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
      ret = true;
      *reason = SUCCEEDED;
      break;

    case STATUS_NG:
      ret = false;
      *reason = header->reason;
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
    case SHOW_GLOBAL_REPLY:
    {
      struct vxlan *vxlan = ( ( show_global_reply * ) reply )->vxlan;
      dump_vxlan_global( vxlan ) ;
    }
    break;

    case LIST_INSTANCES_REPLY:
    {
      unsigned int count = ( unsigned int ) ( header->length - offsetof( list_instances_reply, instances ) ) / sizeof( struct vxlan_instance );
      struct vxlan_instance *instance = ( ( list_instances_reply * ) reply )->instances;
      for ( unsigned int i = 0; i < count; i++ ) {
        dump_vxlan_instance( instance );
        instance++;
      }
    }
    break;

    case SHOW_FDB_REPLY:
    {
      unsigned int count = ( unsigned int ) ( header->length - offsetof( show_fdb_reply, entries ) ) / sizeof( struct fdb_entry );
      struct fdb_entry *entry = ( ( show_fdb_reply * ) reply )->entries;
      for ( unsigned int i = 0; i < count; i++ ) {
        dump_fdb_entry( entry );
        entry++;
      }
    }
    break;

    default:
      break;
  }

  return true;
}


static bool
recv_reply( uint32_t xid, uint16_t set_bitmap, uint8_t *reason ) {
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
      ret = false;
      break;
    }
    command_reply_header *header = ( command_reply_header * ) reply;
    if ( xid != header->xid ) {
      ret = false;
      break;
    }

    if ( n_replies == 0 && header->reason == SUCCEEDED &&
         ( set_bitmap & DISABLE_HEADER ) == 0) {
      switch ( header->type ) {
        case SHOW_GLOBAL_REPLY:
          print_dump_vxlan_global_header();
          break;
        case LIST_INSTANCES_REPLY:
          print_dump_vxlan_instance_header();
          break;
        case SHOW_FDB_REPLY:
          print_dump_fdb_entry_header();
          break;
        default:
          break;
      }
    }

    ret &= handle_reply( ( void * ) reply, length, reason );
    flags = header->flags;
    n_replies++;
  }

  return ret;
}


bool
add_instance( uint32_t vni, struct in_addr addr, uint16_t port, time_t aging_time, uint8_t *reason ) {
  assert( fd >= 0 );
  assert( reason != NULL );

  add_instance_request request;
  memset( &request, 0, sizeof( add_instance_request ) );
  request.header.xid = ( uint32_t ) rand();
  request.header.type = ADD_INSTANCE_REQUEST;
  request.header.length = ( uint32_t ) sizeof( add_instance_request );
  request.instance.vni[ 0 ] = ( uint8_t ) ( ( vni >> 16 ) & 0xff );
  request.instance.vni[ 1 ] = ( uint8_t ) ( ( vni >> 8 ) & 0xff );
  request.instance.vni[ 2 ] = ( uint8_t ) ( vni & 0xff );
  request.instance.addr.sin_addr = addr;
  request.instance.port = port;
  request.instance.aging_time = aging_time;
  size_t length = sizeof( add_instance_request );

  ssize_t ret = send_request( ( void * ) &request, &length );
  if ( ret < 0 ) {
    *reason = OTHER_ERROR;
    return false;
  }

  return recv_reply( request.header.xid, 0, reason);
}


bool
set_instance( uint32_t vni, uint16_t set_bitmap, struct in_addr addr, uint16_t port, time_t aging_time,
              uint8_t *reason ) {
  assert( fd >= 0 );
  assert( reason != NULL );

  set_instance_request request;
  memset( &request, 0, sizeof( set_instance_request ) );
  request.header.xid = ( uint32_t ) rand();
  request.header.type = SET_INSTANCE_REQUEST;
  request.header.length = ( uint32_t ) sizeof( set_instance_request );
  request.set_bitmap = set_bitmap;
  request.instance.vni[ 0 ] = ( uint8_t ) ( ( vni >> 16 ) & 0xff );
  request.instance.vni[ 1 ] = ( uint8_t ) ( ( vni >> 8 ) & 0xff );
  request.instance.vni[ 2 ] = ( uint8_t ) ( vni & 0xff );
  request.instance.addr.sin_addr = addr;
  request.instance.port = port;
  request.instance.aging_time = aging_time;
  size_t length = sizeof( set_instance_request );

  ssize_t ret = send_request( ( void * ) &request, &length );
  if ( ret < 0 ) {
    *reason = OTHER_ERROR;
    return false;
  }

  return recv_reply( request.header.xid, 0, reason );
}


bool
inactivate_instance( uint32_t vni, uint8_t *reason ) {
  assert( fd >= 0 );
  assert( reason != NULL );

  inactivate_instance_request request;
  memset( &request, 0, sizeof( inactivate_instance_request ) );
  request.header.xid = ( uint32_t ) rand();
  request.header.type = INACTIVATE_INSTANCE_REQUEST;
  request.header.length = ( uint32_t ) sizeof( inactivate_instance_request );
  request.vni = vni;
  size_t length = sizeof( inactivate_instance_request );

  ssize_t ret = send_request( ( void * ) &request, &length );
  if ( ret < 0 ) {
    *reason = OTHER_ERROR;
    return false;
  }

  return recv_reply( request.header.xid, 0, reason );
}


bool
activate_instance( uint32_t vni, uint8_t *reason ) {
  assert( fd >= 0 );
  assert( reason != NULL );

  activate_instance_request request;
  memset( &request, 0, sizeof( activate_instance_request ) );
  request.header.xid = ( uint32_t ) rand();
  request.header.type = ACTIVATE_INSTANCE_REQUEST;
  request.header.length = ( uint32_t ) sizeof( activate_instance_request );
  request.vni = vni;
  size_t length = sizeof( activate_instance_request );

  ssize_t ret = send_request( ( void * ) &request, &length );
  if ( ret < 0 ) {
    *reason = OTHER_ERROR;
    return false;
  }

  return recv_reply( request.header.xid, 0, reason );
}


bool
delete_instance( uint32_t vni, uint8_t *reason ) {
  assert( fd >= 0 );
  assert( reason != NULL );

  del_instance_request request;
  memset( &request, 0, sizeof( del_instance_request ) );
  request.header.xid = ( uint32_t ) rand();
  request.header.type = DEL_INSTANCE_REQUEST;
  request.header.length = ( uint32_t ) sizeof( del_instance_request );
  request.vni = vni;
  size_t length = sizeof( del_instance_request );

  ssize_t ret = send_request( ( void * ) &request, &length );
  if ( ret < 0 ) {
    *reason = OTHER_ERROR;
    return false;
  }

  return recv_reply( request.header.xid, 0, reason );
}


bool
show_global( uint16_t set_bitmap, uint8_t *reason ) {
  assert( fd >= 0 );
  assert( reason != NULL );

  show_global_request request;
  memset( &request, 0, sizeof( show_global_request ) );
  request.header.xid = ( uint32_t ) rand();
  request.header.type = SHOW_GLOBAL_REQUEST;
  request.header.length = ( uint32_t ) sizeof( show_global_request );
  size_t length = sizeof( show_global_request );

  ssize_t ret = send_request( ( void * ) &request, &length );
  if ( ret < 0 ) {
    *reason = OTHER_ERROR;
    return false;
  }

  return recv_reply( request.header.xid, set_bitmap, reason );
}


bool
list_instances( uint32_t vni, uint16_t set_bitmap, uint8_t *reason ) {
  assert( fd >= 0 );
  assert( reason != NULL );

  if ( set_bitmap & SHOW_GLOBAL ) {
    show_global( set_bitmap, reason );
    finalize_vxlan_ctrl_client();
    fd = -1;
    init_vxlan_ctrl_client();
  }

  list_instances_request request;
  memset( &request, 0, sizeof( list_instances_request ) );
  request.header.xid = ( uint32_t ) rand();
  request.header.type = LIST_INSTANCES_REQUEST;
  request.header.length = ( uint32_t ) sizeof( list_instances_request );
  request.vni = vni;
  size_t length = sizeof( list_instances_request );

  ssize_t ret = send_request( ( void * ) &request, &length );
  if ( ret < 0 ) {
    *reason = OTHER_ERROR;
    return false;
  }

  return recv_reply( request.header.xid, set_bitmap, reason );
}


bool
show_fdb( uint32_t vni, uint8_t *reason ) {
  assert( fd >= 0 );
  assert( reason != NULL );

  show_fdb_request request;
  memset( &request, 0, sizeof( show_fdb_request ) );
  request.header.xid = ( uint32_t ) rand();
  request.header.type = SHOW_FDB_REQUEST;
  request.header.length = ( uint32_t ) sizeof( show_fdb_request );
  request.vni = vni;
  size_t length = sizeof( show_fdb_request );

  ssize_t ret = send_request( ( void * ) &request, &length );
  if ( ret < 0 ) {
    *reason = OTHER_ERROR;
    return false;
  }

  return recv_reply( request.header.xid, 0, reason );
}


bool
add_fdb_entry( uint32_t vni, struct ether_addr eth_addr, struct in_addr ip_addr, time_t aging_time,
               uint8_t *reason ) {
  assert( fd >= 0 );
  assert( reason != NULL );

  add_fdb_entry_request request;
  memset( &request, 0, sizeof( add_fdb_entry_request ) );
  request.header.xid = ( uint32_t ) rand();
  request.header.type = ADD_FDB_ENTRY_REQUEST;
  request.header.length = ( uint32_t ) sizeof( add_fdb_entry_request );
  request.vni = vni;
  request.eth_addr = eth_addr;
  request.ip_addr = ip_addr;
  request.aging_time = aging_time;
  size_t length = sizeof( add_fdb_entry_request );

  ssize_t ret = send_request( ( void * ) &request, &length );
  if ( ret < 0 ) {
    *reason = OTHER_ERROR;
    return false;
  }

  return recv_reply( request.header.xid, 0, reason );
}


bool
delete_fdb_entry( uint32_t vni, struct ether_addr eth_addr, uint8_t *reason ) {
  assert( fd >= 0 );
  assert( reason != NULL );

  del_fdb_entry_request request;
  memset( &request, 0, sizeof( del_fdb_entry_request ) );
  request.header.xid = ( uint32_t ) rand();
  request.header.type = DEL_FDB_ENTRY_REQUEST;
  request.header.length = ( uint32_t ) sizeof( del_fdb_entry_request );
  request.vni = vni;
  request.eth_addr = eth_addr;
  size_t length = sizeof( del_fdb_entry_request );

  ssize_t ret = send_request( ( void * ) &request, &length );
  if ( ret < 0 ) {
    *reason = OTHER_ERROR;
    return false;
  }

  return recv_reply( request.header.xid, 0, reason );
}


bool
init_vxlan_ctrl_client() {
  assert( fd < 0 );

  return init_ctrl_client( &fd );
}


bool
finalize_vxlan_ctrl_client() {
  assert( fd >= 0 );

  return finalize_ctrl_client( fd );
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
