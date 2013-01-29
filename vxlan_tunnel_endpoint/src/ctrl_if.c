/*
 * Copyright (C) 2012-2013 NEC Corporation
 * NEC Confidential
 */


#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>
#include "ctrl_if.h"
#include "log.h"
#include "wrapper.h"


extern volatile bool running;
static const int MAX_SEND_RECV_RETRY = 5;


ssize_t
send_command( int fd, void *command, size_t *length ) {
  assert( command != NULL );
  assert( length != NULL );
  assert( *length > 0 );
  assert( fd >= 0 );

  int n_retries = 0;
  ssize_t sent_length = 0;

  while ( running && ( n_retries < MAX_SEND_RECV_RETRY ) ) {
    fd_set fdset;
    FD_ZERO( &fdset );
    FD_SET( fd, &fdset );

    struct timespec timeout = { 1, 0 };

    ssize_t retval = -1;
    int ret = pselect( fd + 1, NULL, &fdset, NULL, &timeout, NULL );
    if ( ret < 0 ) {
      if ( errno == EAGAIN || errno == EINTR ) {
        continue;
      }
      error( "Failed to select ( fd = %d, errno = %d ).", fd, errno );
      break;
    }
    else if ( ret == 0 ) {
      break;
    }

    if ( !FD_ISSET( fd, &fdset ) ) {
      continue;
    }

    retval = send( fd, command, *length, MSG_DONTWAIT );
    if ( retval < 0 ) {
      if ( errno == EAGAIN || errno == EINTR || errno == EWOULDBLOCK ) {
        n_retries++;
        continue;
      }
      error( "Failed to send ( fd = %d, errno = %d ).", fd, errno );
      break;
    }
    else if ( retval == 0 ) {
      n_retries++;
      continue;
    }

    command = ( void * ) ( ( char * ) command + retval );
    sent_length += retval;
    *length -= ( size_t ) retval;
    if ( *length > 0 ) {
      n_retries++;
      continue;
    }
    break;
  }

  return sent_length;
}


ssize_t
recv_command( int fd, void *command, size_t *length ) {
  assert( command != NULL );
  assert( length != NULL );
  assert( *length > 0 );
  assert( fd >= 0 );

  int n_retries = 0;
  ssize_t retval = -1;

  while ( running && ( n_retries < MAX_SEND_RECV_RETRY ) ) {
    fd_set fdset;
    FD_ZERO( &fdset );
    FD_SET( fd, &fdset );

    struct timespec timeout = { 1, 0 };

    int ret = pselect( fd + 1, &fdset, NULL, NULL, &timeout, NULL );
    if ( ret < 0 ) {
      if ( errno == EAGAIN || errno == EINTR ) {
        continue;
      }
      error( "Failed to select ( fd = %d, errno = %d ).", fd, errno );
      break;
    }
    else if ( ret == 0 ) {
      break;
    }

    if ( !FD_ISSET( fd, &fdset ) ) {
      continue;
    }

    retval = recv( fd, command, *length, MSG_DONTWAIT );
    if ( retval < 0 ) {
      if ( errno == EAGAIN || errno == EINTR || errno == EWOULDBLOCK ) {
        n_retries++;
        continue;
      }
      error( "Failed to recv ( fd = %d, errno = %d ).", fd, errno );
      break;
    }
    else if ( retval == 0 ) {
      // connection closed by peer
      break;
    }
    *length = ( size_t ) retval;
    break;
  }

  return retval;
}


bool
init_ctrl_client( int *fd ) {
  assert( fd != NULL );

  *fd = socket( PF_UNIX, SOCK_SEQPACKET, 0 );
  if ( *fd < 0 ) {
    char buf[ 256 ];
    char *error_string = safe_strerror_r( errno, buf, sizeof( buf ) );
    error( "Failed to open socket ( ret = %d, errno = %s [%d] ).", *fd, error_string, errno );
    return false;
  }

  srand( ( unsigned int ) time( NULL ) );

  return true;
}


bool
finalize_ctrl_client( int fd ) {
  assert( fd >= 0 );

  close( fd );

  return true;
}


bool
init_ctrl_server( int *listen_fd, const char *file ) {
  assert( listen_fd != NULL );

  char buf[ 256 ];

  *listen_fd = socket( PF_UNIX, SOCK_SEQPACKET, 0 );
  if ( *listen_fd < 0 ) {
    char *error_string = safe_strerror_r( errno, buf, sizeof( buf ) );
    error( "Failed to open socket ( ret = %d, errno = %s [%d] ).", *listen_fd, error_string, errno );
    return false;
  }

  struct sockaddr_un saddr;
  memset( &saddr, 0, sizeof( saddr ) );
  saddr.sun_family = AF_UNIX;
  memset( saddr.sun_path, '\0', sizeof( saddr.sun_path ) );
  strncpy( saddr.sun_path, file, sizeof( saddr.sun_path ) - 1 );

  unlink( file );
  int ret = bind( *listen_fd, ( struct sockaddr * ) &saddr, sizeof( saddr ) );
  if ( ret < 0 ) {
    char *error_string = safe_strerror_r( errno, buf, sizeof( buf ) );
    error( "Failed to bind ( ret = %d, errno = %s [%d] ).", ret, error_string, errno );
    close( *listen_fd );
    return false;
  }

  ret = listen( *listen_fd, 1 );
  if ( ret < 0 ) {
    char *error_string = safe_strerror_r( errno, buf, sizeof( buf ) );
    error( "Failed to listen ( ret = %d, errno = %s [%d] ).", ret, error_string, errno );
    close( *listen_fd );
  }

  return true;
}


bool
finalize_ctrl_server( int listen_fd, const char *file ) {
  assert( listen_fd >= 0 );

  close( listen_fd );
  unlink( file );

  return true;
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
