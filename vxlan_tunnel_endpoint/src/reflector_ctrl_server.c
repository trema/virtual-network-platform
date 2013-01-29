/*
 * Copyright (C) 2012-2013 NEC Corporation
 * NEC Confidential
 */


#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/limits.h>
#include <net/if_arp.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>
#include "reflector_ctrl_server.h"
#include "distributor.h"
#include "ethdev.h"
#include "log.h"
#include "wrapper.h"


static int listen_fd = -1;
static ethdev *dev = NULL;


static ssize_t
send_reply( int fd, void *reply, size_t *length ) {
  return send_command( fd, reply, length );
}


static ssize_t
recv_request( int fd, void *request, size_t *length ) {
  return recv_command( fd, request, length );
}


static void
add_tep( int fd, add_tep_request *request ) {
  assert( fd >= 0 );
  assert( request != NULL );

  add_tep_reply reply;
  size_t length = sizeof( add_tep_reply );
  memset( &reply, 0, length );
  reply.header.xid = request->header.xid;
  reply.header.type = ADD_TEP_REPLY;

  bool ret = true;
  if ( !( valid_vni( request->vni ) || request->vni == VNI_ANY ) ) {
    reply.header.reason = INVALID_ARGUMENT;
  }
  else {
    ret = add_tunnel_endpoint( request->vni, request->ip_addr, request->port );
    if ( !ret ) {
      reply.header.reason = DUPLICATED_TEP_ENTRY;
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
set_tep( int fd, set_tep_request *request ) {
  assert( fd >= 0 );
  assert( request != NULL );

  set_tep_reply reply;
  size_t length = sizeof( set_tep_reply );
  memset( &reply, 0, length );
  reply.header.xid = request->header.xid;
  reply.header.type = SET_TEP_REPLY;

  bool ret = true;
  if ( !( valid_vni( request->vni ) || request->vni == VNI_ANY ) ) {
    reply.header.reason = INVALID_ARGUMENT;
    ret = false;
  }
  else {
    if ( request->set_bitmap & SET_TEP_PORT ) {
      ret = set_tunnel_endpoint_port( request->vni, request->ip_addr, request->port );
      if ( !ret ) {
        reply.header.reason = TEP_ENTRY_NOT_FOUND;
      }
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
delete_tep( int fd, del_tep_request *request ) {
  assert( fd >= 0 );
  assert( request != NULL );

  del_tep_reply reply;
  size_t length = sizeof( del_tep_reply );
  memset( &reply, 0, length );
  reply.header.xid = request->header.xid;
  reply.header.type = DEL_TEP_REPLY;

  bool ret = true;
  if ( !( valid_vni( request->vni ) || request->vni == VNI_ANY ) ) {
    reply.header.reason = INVALID_ARGUMENT;
    ret = false;
  }
  else {
    ret = delete_tunnel_endpoint( request->vni, request->ip_addr );
    if ( !ret ) {
      reply.header.reason = TEP_ENTRY_NOT_FOUND;
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
list_tep( int fd, list_tep_request *request ) {
  assert( fd >= 0 );
  assert( request != NULL );

  uint8_t reason = 0;
  list *l = NULL;
  bool ret = true;
  if ( !( valid_vni( request->vni ) || request->vni == VNI_ANY ) ) {
    reason = INVALID_ARGUMENT;
    ret = false;
  }
  else {
    if ( request->vni == VNI_ANY ) {
      l = get_all_tunnel_endpoints();
    }
    else {
      l = lookup_tunnel_endpoints( request->vni );
      if ( l == NULL || ( l != NULL && l->head == NULL ) ) {
        reason = TEP_ENTRY_NOT_FOUND;
        ret = false;
      }
    }
  }

  if ( l == NULL || ( l != NULL && l->head == NULL ) ) {
    size_t length = offsetof( list_tep_reply, tep );
    list_tep_reply *reply = malloc( length );
    memset( reply, 0, length );
    reply->header.xid = request->header.xid;
    reply->header.type = LIST_TEP_REPLY;
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
    free( reply );
    return;
  }

  pthread_mutex_lock( &l->mutex );
  for ( list_element *e = l->head; e != NULL; e = e->next ) {
    if ( e->data == NULL ) {
      continue;
    }
    size_t length = offsetof( list_tep_reply, tep ) + sizeof( tunnel_endpoint );
    list_tep_reply *reply = malloc( length );
    memset( reply, 0, length );
    reply->header.xid = request->header.xid;
    reply->header.type = LIST_TEP_REPLY;
    reply->header.status = STATUS_OK;
    reply->header.flags = ( ( e->next != NULL && e->next->data != NULL ) ? FLAG_MORE : FLAG_NONE );
    reply->header.length = ( uint16_t ) length;
    memcpy( reply->tep, e->data, sizeof( tunnel_endpoint ) );
    send_reply( fd, ( void * ) reply, &length );
    free( reply );
  }

  if ( request->vni == VNI_ANY ) {
    for ( list_element *e = l->head; e != NULL; e = e->next ) {
      if ( e->data != NULL ) {
        free( e->data );
      }
    }
  }

  pthread_mutex_unlock( &l->mutex );

  if ( request->vni == VNI_ANY ) {
    delete_list( l );
  }
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
    case ADD_TEP_REQUEST:
      add_tep( fd, request );
      break;

    case SET_TEP_REQUEST:
      set_tep( fd, request );
      break;

    case DEL_TEP_REQUEST:
      delete_tep( fd, request );
      break;

    case LIST_TEP_REQUEST:
      list_tep( fd, request );
      break;

    default:
      error( "Unhandled message type ( %#x ).", type );
      return false;
  }

  return true;
}


bool
run_reflector_ctrl_server() {
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

  return false;
}


bool
init_reflector_ctrl_server( ethdev *_dev ) {
  assert( listen_fd < 0 );
  assert( _dev != NULL );

  dev = _dev;

  return init_ctrl_server( &listen_fd, CTRL_SERVER_SOCK_FILE );
}


bool
finalize_reflector_ctrl_server() {
  assert( listen_fd >= 0 );
  assert( dev != NULL );

  dev = NULL;

  return finalize_ctrl_server( listen_fd, CTRL_SERVER_SOCK_FILE );
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
