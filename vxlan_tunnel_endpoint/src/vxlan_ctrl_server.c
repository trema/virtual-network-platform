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
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>
#include "checks.h"
#include "fdb.h"
#include "vxlan_instance.h"
#include "vxlan_ctrl_server.h"
#include "linked_list.h"
#include "log.h"
#include "wrapper.h"


static struct vxlan *vxlan = NULL;
static int listen_fd = -1;


static ssize_t
send_reply( int fd, void *reply, size_t *length ) {
  return send_command( fd, reply, length );
}


static ssize_t
recv_request( int fd, void *request, size_t *length ) {
  return recv_command( fd, request, length );
}


static void
add_instance( int fd, add_instance_request *request ) {
  assert( fd >= 0 );
  assert( request != NULL );

  add_instance_reply reply;
  size_t length = sizeof( add_instance_reply );
  memset( &reply, 0, length );
  reply.header.xid = request->header.xid;
  reply.header.type = ADD_INSTANCE_REPLY;
  reply.header.reason = SUCCEEDED;

  bool ret = true;
  struct vxlan_instance *instance = search_hash( &vxlan->instances, request->instance.vni );
  if ( instance != NULL ) {
    reply.header.reason = DUPLICATED_INSTANCE;
    ret = false;
  }
  else {
    instance = create_vxlan_instance( request->instance.vni,
                                      request->instance.addr.sin_addr,
                                      request->instance.port,
                                      request->instance.aging_time );
    if ( instance == NULL ) {
      reply.header.reason = INVALID_ARGUMENT;
      ret = false;
    }
    insert_hash( &vxlan->instances, instance, request->instance.vni );
    start_vxlan_instance( instance );
    vxlan->n_instances++;

    uint32_t vni32 = ( uint32_t ) request->instance.vni[ 2 ];
    vni32 |= ( uint32_t ) ( request->instance.vni[ 1 ] << 8 );
    vni32 |= ( uint32_t ) ( request->instance.vni[ 0 ] << 16 );

    debug( "A VXLAN instance is created ( vni = %#x ).", vni32 );
  }

  if ( ret ) {
    reply.header.status = STATUS_OK;
  }
  else {
    reply.header.status = STATUS_NG;
  }
  reply.header.flags = FLAG_NONE;
  reply.header.length = ( uint16_t ) length;
  send_reply( fd, ( void * ) &reply, &length );
}


static void
set_instance( int fd, set_instance_request *request ) {
  assert( fd >= 0 );
  assert( request != NULL );

  set_instance_reply reply;
  size_t length = sizeof( set_instance_reply );
  memset( &reply, 0, length );
  reply.header.xid = request->header.xid;
  reply.header.type = SET_INSTANCE_REPLY;
  reply.header.reason = SUCCEEDED;

  bool ret = true;
  struct vxlan_instance *instance = search_hash( &vxlan->instances, request->instance.vni );
  if ( instance != NULL ) {
    if ( request->set_bitmap & SET_IP_ADDR ) {
      ret &= set_vxlan_instance_flooding_addr( request->instance.vni, request->instance.addr.sin_addr );
    }
    if ( ret && ( request->set_bitmap & SET_UDP_PORT ) != 0 ) {
      ret &= set_vxlan_instance_port( request->instance.vni, request->instance.port );
    }
    if ( ret && ( request->set_bitmap & SET_AGING_TIME ) != 0 ) {
      ret &= set_vxlan_instance_aging_time( request->instance.vni, request->instance.aging_time );
    }
    if ( !ret ) {
      reply.header.reason = INVALID_ARGUMENT;
    }
  }
  else {
    reply.header.reason = INSTANCE_NOT_FOUND;
    ret = false;
  }

  if ( ret ) {
    reply.header.status = STATUS_OK;
  }
  else {
    reply.header.status = STATUS_NG;
  }
  reply.header.flags = FLAG_NONE;
  reply.header.length = ( uint16_t ) length;
  send_reply( fd, ( void * ) &reply, &length );
}


static void
inactivate_instance( int fd, inactivate_instance_request *request ) {
  assert( fd >= 0 );
  assert( request != NULL );

  inactivate_instance_reply reply;
  size_t length = sizeof( inactivate_instance_reply );
  memset( &reply, 0, length );
  reply.header.xid = request->header.xid;
  reply.header.type = INACTIVATE_INSTANCE_REPLY;
  reply.header.reason = SUCCEEDED;

  uint8_t vni[ VXLAN_VNISIZE ];
  vni[ 0 ] = ( uint8_t ) ( ( request->vni >> 16 ) & 0xff );
  vni[ 1 ] = ( uint8_t ) ( ( request->vni >> 8 ) & 0xff );
  vni[ 2 ] = ( uint8_t ) ( request->vni & 0xff );

  bool ret = true;
  struct vxlan_instance *instance = search_hash( &vxlan->instances, vni );
  if ( instance == NULL ) {
    reply.header.reason = INSTANCE_NOT_FOUND;
    ret = false;
  }
  else {
    ret = inactivate_vxlan_instance( vni );
    if ( !ret ) {
      reply.header.reason = OTHER_ERROR;
      error( "Failed to inactivate an VXLAN instance ( vni = %#x ).", request->vni );
    }
    else {
      debug( "A VXLAN instance is inactivated ( vni = %#x ).", request->vni );
    }
  }

  if ( ret ) {
    reply.header.status = STATUS_OK;
  }
  else {
    reply.header.status = STATUS_NG;
  }
  reply.header.flags = FLAG_NONE;
  reply.header.length = ( uint16_t ) length;
  send_reply( fd, ( void * ) &reply, &length );
}


static void
activate_instance( int fd, activate_instance_request *request ) {
  assert( fd >= 0 );
  assert( request != NULL );

  activate_instance_reply reply;
  size_t length = sizeof( activate_instance_reply );
  memset( &reply, 0, length );
  reply.header.xid = request->header.xid;
  reply.header.type = ACTIVATE_INSTANCE_REPLY;
  reply.header.reason = SUCCEEDED;

  uint8_t vni[ VXLAN_VNISIZE ];
  vni[ 0 ] = ( uint8_t ) ( ( request->vni >> 16 ) & 0xff );
  vni[ 1 ] = ( uint8_t ) ( ( request->vni >> 8 ) & 0xff );
  vni[ 2 ] = ( uint8_t ) ( request->vni & 0xff );

  bool ret = true;
  struct vxlan_instance *instance = search_hash( &vxlan->instances, vni );
  if ( instance == NULL ) {
    reply.header.reason = INSTANCE_NOT_FOUND;
    ret = false;
  }
  else {
    ret = activate_vxlan_instance( vni );
    if ( !ret ) {
      reply.header.reason = OTHER_ERROR;
      error( "Failed to activate an VXLAN instance ( vni = %#x ).", request->vni );
    }
    else {
      debug( "A VXLAN instance is activated ( vni = %#x ).", request->vni );
    }
  }

  if ( ret ) {
    reply.header.status = STATUS_OK;
  }
  else {
    reply.header.status = STATUS_NG;
  }
  reply.header.flags = FLAG_NONE;
  reply.header.length = ( uint16_t ) length;
  send_reply( fd, ( void * ) &reply, &length );
}


static void
delete_instance( int fd, del_instance_request *request ) {
  assert( fd >= 0 );
  assert( request != NULL );

  del_instance_reply reply;
  size_t length = sizeof( del_instance_reply );
  memset( &reply, 0, length );
  reply.header.xid = request->header.xid;
  reply.header.type = DEL_INSTANCE_REPLY;
  reply.header.reason = SUCCEEDED;

  uint8_t vni[ VXLAN_VNISIZE ];
  vni[ 0 ] = ( uint8_t ) ( ( request->vni >> 16 ) & 0xff );
  vni[ 1 ] = ( uint8_t ) ( ( request->vni >> 8 ) & 0xff );
  vni[ 2 ] = ( uint8_t ) ( request->vni & 0xff );

  bool ret = true;
  struct vxlan_instance *instance = search_hash( &vxlan->instances, vni );
  if ( instance == NULL ) {
    reply.header.reason = INSTANCE_NOT_FOUND;
    ret = false;
  }
  else {
    ret = destroy_vxlan_instance( instance );
    if ( ret ) {
      debug( "A VXLAN instance is deleted ( vni = %#x ).", request->vni );
    }
    else {
      reply.header.reason = OTHER_ERROR;
      ret = false;
    }
  }

  if ( ret ) {
    reply.header.status = STATUS_OK;
  }
  else {
    reply.header.status = STATUS_NG;
  }
  reply.header.flags = FLAG_NONE;
  reply.header.length = ( uint16_t ) length;
  send_reply( fd, ( void * ) &reply, &length );
}


static void
show_global( int fd, show_global_request *request ) {
  assert( fd >= 0 );
  assert( vxlan != NULL );
  assert( request != NULL );

  size_t length = offsetof( show_global_reply, vxlan ) + sizeof( struct vxlan );
  show_global_reply *reply = malloc( length );
  memset( reply, 0, length );
  reply->header.xid = request->header.xid;
  reply->header.type = SHOW_GLOBAL_REPLY;
  reply->header.reason = SUCCEEDED;
  reply->header.flags = FLAG_NONE;
  reply->header.length = ( uint16_t ) length;
  memcpy( reply->vxlan, vxlan, sizeof( struct vxlan ) );
  send_reply( fd, ( void * ) reply, &length );
  free( reply );
  return;
}


static void
list_instances( int fd, list_instances_request *request ) {
  assert( fd >= 0 );
  assert( request != NULL );

  bool ret = true;
  int n_instances = 0;
  struct vxlan_instance **instances = NULL;
  if ( request->vni == 0xffffffff ) {
    instances = get_all_vxlan_instances( &n_instances );
  } else {
    uint8_t vni[ VXLAN_VNISIZE ];
    vni[ 0 ] = ( uint8_t ) ( ( request->vni >> 16 ) & 0xff );
    vni[ 1 ] = ( uint8_t ) ( ( request->vni >> 8 ) & 0xff );
    vni[ 2 ] = ( uint8_t ) ( request->vni & 0xff );
    struct vxlan_instance *instance = search_hash( &vxlan->instances, vni );
    if ( instance ) {
      n_instances = 1;
      instances = malloc( sizeof( struct vxlan_instance * ) );
      *instances = instance;
    }
  }
  if ( instances == NULL || n_instances == 0 ) {
    size_t length = offsetof( list_instances_reply, instances );
    list_instances_reply *reply = malloc( length );
    memset( reply, 0, length );
    reply->header.xid = request->header.xid;
    reply->header.type = LIST_INSTANCES_REPLY;
    if ( ret ) {
      reply->header.status = STATUS_OK;
      reply->header.reason = SUCCEEDED;
    }
    else {
      reply->header.status = STATUS_NG;
      reply->header.reason = OTHER_ERROR;
    }
    reply->header.flags = FLAG_NONE;
    reply->header.length = ( uint16_t ) length;
    send_reply( fd, ( void * ) reply, &length );
    if ( instances != NULL ) {
      free( instances );
    }
    free( reply );
    return;
  }

  for ( int n = 0; n < n_instances; n++ ) {
    size_t length = offsetof( list_instances_reply, instances ) + sizeof( struct vxlan_instance );
    list_instances_reply *reply = malloc( length );
    memset( reply, 0, length );
    reply->header.xid = request->header.xid;
    reply->header.type = LIST_INSTANCES_REPLY;
    reply->header.status = STATUS_OK;
    reply->header.reason = SUCCEEDED;
    reply->header.flags = ( n == ( n_instances - 1 ) ) ? FLAG_NONE : FLAG_MORE;
    reply->header.length = ( uint16_t ) length;
    memcpy( reply->instances, instances[ n ], sizeof( struct vxlan_instance ) );
    send_reply( fd, ( void * ) reply, &length );
    free( reply );
  }

  if ( instances != NULL ) {
    free( instances );
  }
}


static void
show_fdb( int fd, show_fdb_request *request ) {
  assert( fd >= 0 );
  assert( vxlan != NULL );
  assert( request != NULL );

  uint8_t vni[ VXLAN_VNISIZE ];
  vni[ 0 ] = ( uint8_t ) ( ( request->vni >> 16 ) & 0xff );
  vni[ 1 ] = ( uint8_t ) ( ( request->vni >> 8 ) & 0xff );
  vni[ 2 ] = ( uint8_t ) ( request->vni & 0xff );

  bool ret = true;
  struct vxlan_instance *instance = search_hash( &vxlan->instances, vni );
  uint8_t reason = SUCCEEDED;
  list *entries = NULL;
  if ( instance != NULL ) {
    if ( instance->fdb != NULL ) {
      entries = get_fdb_entries( instance->fdb );
    }
    else {
      reason = OTHER_ERROR;
      ret = false;
    }
  }
  else {
    reason = INSTANCE_NOT_FOUND;
    ret = false;
  }
  if ( !ret || entries == NULL || ( entries != NULL && entries->head == NULL ) ) {
    size_t length = offsetof( show_fdb_reply, entries );
    show_fdb_reply *reply = malloc( length );
    memset( reply, 0, length );
    reply->header.xid = request->header.xid;
    reply->header.type = SHOW_FDB_REPLY;
    if ( ret ) {
      reply->header.status = STATUS_OK;
    }
    else {
      reply->header.status = STATUS_NG;
    }
    reply->header.reason = reason;
    reply->header.flags = FLAG_NONE;
    reply->header.length = ( uint16_t ) length;
    send_reply( fd, ( void * ) reply, &length );
    if ( entries != NULL ) {
      delete_list( entries );
    }
    free( reply );
    return;
  }

  for ( list_element *e = entries->head; e != NULL; e = e->next ) {
    size_t length = offsetof( show_fdb_reply, entries ) + sizeof( struct fdb_entry );
    show_fdb_reply *reply = malloc( length );
    memset( reply, 0, length );
    reply->header.xid = request->header.xid;
    reply->header.type = SHOW_FDB_REPLY;
    reply->header.status = STATUS_OK;
    reply->header.reason = SUCCEEDED;
    reply->header.flags = ( e->next == NULL ) ? FLAG_NONE : FLAG_MORE;
    reply->header.length = ( uint16_t ) length;
    memcpy( reply->entries, e->data, sizeof( struct fdb_entry ) );
    send_reply( fd, ( void * ) reply, &length );
    free( reply );
  }

  if ( entries != NULL ) {
    delete_list_totally( entries );
  }
}


static void
add_fdb_entry( int fd, add_fdb_entry_request *request ) {
  assert( fd >= 0 );
  assert( vxlan != NULL );
  assert( request != NULL );

  add_fdb_entry_reply reply;
  size_t length = sizeof( add_fdb_entry_reply );
  memset( &reply, 0, length );
  reply.header.xid = request->header.xid;
  reply.header.type = ADD_FDB_ENTRY_REPLY;
  reply.header.reason = SUCCEEDED;

  uint8_t vni[ VXLAN_VNISIZE ];
  vni[ 0 ] = ( uint8_t ) ( ( request->vni >> 16 ) & 0xff );
  vni[ 1 ] = ( uint8_t ) ( ( request->vni >> 8 ) & 0xff );
  vni[ 2 ] = ( uint8_t ) ( request->vni & 0xff );

  bool ret = true;
  struct vxlan_instance *instance = search_hash( &vxlan->instances, vni );
  if ( instance == NULL ) {
    reply.header.reason = INSTANCE_NOT_FOUND;
    ret = false;
  }
  else {
    if ( instance->fdb != NULL ) {
      ret = fdb_add_static_entry( instance->fdb, request->eth_addr, request->ip_addr, request->aging_time );
      if ( ret ) {
        debug( "A fdb entry is added ( vni = %#x ).", request->vni );
      }
      else {
        reply.header.reason = OTHER_ERROR;
        ret = false;
      }
    }
    else {
      reply.header.reason = OTHER_ERROR;
      ret = false;
    }
  }

  if ( ret ) {
    reply.header.status = STATUS_OK;
  }
  else {
    reply.header.status = STATUS_NG;
  }
  reply.header.flags = FLAG_NONE;
  reply.header.length = ( uint16_t ) length;
  send_reply( fd, ( void * ) &reply, &length );
}


static void
delete_fdb_entry( int fd, del_fdb_entry_request *request ) {
  assert( fd >= 0 );
  assert( vxlan != NULL );
  assert( request != NULL );

  del_fdb_entry_reply reply;
  size_t length = sizeof( del_fdb_entry_reply );
  memset( &reply, 0, length );
  reply.header.xid = request->header.xid;
  reply.header.type = DEL_FDB_ENTRY_REPLY;
  reply.header.reason = SUCCEEDED;

  uint8_t vni[ VXLAN_VNISIZE ];
  vni[ 0 ] = ( uint8_t ) ( ( request->vni >> 16 ) & 0xff );
  vni[ 1 ] = ( uint8_t ) ( ( request->vni >> 8 ) & 0xff );
  vni[ 2 ] = ( uint8_t ) ( request->vni & 0xff );

  bool ret = true;
  struct vxlan_instance *instance = search_hash( &vxlan->instances, vni );
  if ( instance == NULL ) {
    reply.header.reason = INSTANCE_NOT_FOUND;
    ret = false;
  }
  else {
    if ( instance->fdb != NULL ) {
      uint8_t zero[ ETH_ALEN ] = { 0, 0, 0, 0, 0, 0 };
      if ( memcmp( request->eth_addr.ether_addr_octet, zero, ETH_ALEN ) != 0 ) {
        ret = fdb_delete_entry( instance->fdb, request->eth_addr );
        if ( ret ) {
          debug( "A fdb entry is deleted ( vni = %#x ).", request->vni );
        }
        else {
          reply.header.reason = OTHER_ERROR;
          ret = false;
        }
      }
      else {
        ret = fdb_delete_all_entries( instance->fdb, FDB_ENTRY_TYPE_ALL );
        if ( ret ) {
          debug( "All fdb entries are deleted ( vni = %#x ).", request->vni );
        }
        else {
          reply.header.reason = OTHER_ERROR;
          ret = false;
        }
      }
    }
    else {
      reply.header.reason = OTHER_ERROR;
      ret = false;
    }
  }

  if ( ret ) {
    reply.header.status = STATUS_OK;
  }
  else {
    reply.header.status = STATUS_NG;
  }
  reply.header.flags = FLAG_NONE;
  reply.header.length = ( uint16_t ) length;
  send_reply( fd, ( void * ) &reply, &length );
}


static bool
handle_request( int fd, void *request, size_t *length ) {
  assert( fd >= 0 );
  assert( request != NULL );
  assert( length != NULL );
  assert( *length >= sizeof( command_request_header ) );

  command_request_header *header = request;
  uint8_t type = header->type;
  switch ( type ) {
    case ADD_INSTANCE_REQUEST:
      add_instance( fd, request );
      break;

    case SET_INSTANCE_REQUEST:
      set_instance( fd, request );
      break;

    case INACTIVATE_INSTANCE_REQUEST:
      inactivate_instance( fd, request );
      break;

    case ACTIVATE_INSTANCE_REQUEST:
      activate_instance( fd, request );
      break;

    case DEL_INSTANCE_REQUEST:
      delete_instance( fd, request );
      break;

    case SHOW_GLOBAL_REQUEST:
      show_global( fd, request );
      break;

    case LIST_INSTANCES_REQUEST:
      list_instances( fd, request );
      break;

    case SHOW_FDB_REQUEST:
      show_fdb( fd, request );
      break;

    case ADD_FDB_ENTRY_REQUEST:
      add_fdb_entry( fd, request );
      break;

    case DEL_FDB_ENTRY_REQUEST:
      delete_fdb_entry( fd, request );
      break;

    default:
      error( "Unhandled message type ( %#x ).", type );
      return false;
  }

  return true;
}


static void *
run_vxlan_ctrl_server( void *param ) {
  UNUSED( param );
  assert( listen_fd >= 0 );

  uint8_t request[ COMMAND_MESSAGE_LENGTH ];
  size_t length = COMMAND_MESSAGE_LENGTH;

  char buf[ 256 ];

  while ( running ) {
    fd_set fds;
    FD_ZERO( &fds );
    FD_SET( listen_fd, &fds );
    struct timespec timeout = { 1, 0 };
    int ret = pselect( listen_fd + 1, &fds, NULL, NULL, &timeout, NULL );
    if ( ret < 0 ) {
      if ( errno == EINTR ) {
        continue;
      }
      char *error_string = safe_strerror_r( errno, buf, sizeof( buf ) );
      error( "Failed to select ( listen_fd = %d, errno = %s [%d] ).", listen_fd, error_string, errno );
      break;
    }
    else if ( ret == 0 ) {
      continue;
    }

    if ( !FD_ISSET( listen_fd, &fds ) ) {
      continue;
    }

    int fd = accept( listen_fd, NULL, NULL );
    if ( fd < 0 ) {
      if ( errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR ) {
        continue;
      }
      char *error_string = safe_strerror_r( errno, buf, sizeof( buf ) );
      error( "Failed to accept ( listen_fd = %d, errno = %s [%d] ).", listen_fd, error_string, errno );
      break;
    }
    memset( &request, 0, ( size_t ) sizeof( request ) );
    length = COMMAND_MESSAGE_LENGTH;
    ssize_t retval = recv_request( fd, request, &length );
    if ( retval > 0 ) {
      handle_request( fd, ( void * ) request, &length );
    }
    close( fd );
  }

  return NULL;
}


bool
start_vxlan_ctrl_server( ) {
  assert( vxlan != NULL );

  pthread_attr_t attr;

  pthread_attr_init( &attr );
  int ret = pthread_attr_setstacksize( &attr, 1024 * 1024 );
  if ( ret != 0 ) {
    critical( "Failed to set stack size for a control thread." );
    return false;
  }
  ret = pthread_create( &vxlan->control_tid, &attr, run_vxlan_ctrl_server, NULL );
  if ( ret != 0 ) {
    critical( "Failed to create a control thread." );
    return false;
  }

  pthread_create( &vxlan->control_tid, &attr, run_vxlan_ctrl_server, NULL );

  return true;
}


bool
init_vxlan_ctrl_server( struct vxlan *_vxlan ){
  assert( listen_fd < 0 );
  assert( _vxlan != NULL );

  vxlan = _vxlan;

  return init_ctrl_server( &listen_fd, CTRL_SERVER_SOCK_FILE );
}


bool
finalize_vxlan_ctrl_server() {
  assert( listen_fd >= 0 );
  assert( vxlan != NULL );

  return finalize_ctrl_server( listen_fd, CTRL_SERVER_SOCK_FILE );
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
