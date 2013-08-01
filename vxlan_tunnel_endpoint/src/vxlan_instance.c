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
#include <limits.h>
#include <net/if.h>
#include <netdb.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/timerfd.h>
#include <sys/types.h>
#include <syslog.h>
#include <unistd.h>
#include "fdb.h"
#include "iftap.h"
#include "log.h"
#include "net.h"
#include "vxlan_instance.h"
#include "wrapper.h"


static struct vxlan *vxlan = NULL;


struct vxlan_instance *
create_vxlan_instance( uint8_t *vni, struct in_addr addr, uint16_t port, time_t aging_time ) {
  assert( vxlan != NULL );
  assert( vni != NULL );

  struct vxlan_instance *instance = malloc( sizeof( struct vxlan_instance ) );
  memset( instance, 0, sizeof( struct vxlan_instance ) );
  memcpy( instance->vni, vni, sizeof( instance->vni ) );
  if ( ntohl( addr.s_addr ) != INADDR_ANY ) {
    instance->addr.sin_addr = addr;
  }
  else {
    instance->addr.sin_addr = vxlan->flooding_addr;
  }

  if ( port > 0 ) {
    instance->port = port;
  }
  else {
    instance->port = vxlan->flooding_port;
  }
  instance->addr.sin_port = htons( instance->port );

  if ( aging_time >= 0 && aging_time <= VXLAN_MAX_AGING_TIME ) {
    instance->aging_time = aging_time;
  }
  else {
    instance->aging_time = vxlan->aging_time;
  }
  instance->tap_sock = -1;
  instance->activated = false;

  instance->udp_sock = vxlan->udp_sock;
  if ( !IN_MULTICAST( ntohl( instance->addr.sin_addr.s_addr ) ) ) {
    instance->multicast_joined = true;
  }
  else {
    instance->multicast_joined = false;
  }

  uint32_t vni32 = ( uint32_t ) ( instance->vni[ 0 ] << 16 );
  vni32 |= ( uint32_t ) ( instance->vni[ 1 ] << 8 );
  vni32 |= ( uint32_t ) instance->vni[ 2 ];
  memset( instance->vxlan_tap_name, '\0', sizeof( instance->vxlan_tap_name ) );
  snprintf( instance->vxlan_tap_name, sizeof( instance->vxlan_tap_name ) - 1, "vxlan%u", vni32 );

  instance->fdb = NULL;
  instance->tap_sock = tap_alloc( instance->vxlan_tap_name );

  return instance;
}


static bool
multicast_join( struct vxlan_instance *instance ) {
  assert( instance != NULL );

  if ( instance->multicast_joined ) {
    return true;
  }

  instance->multicast_joined = ipv4_multicast_join( instance->addr.sin_addr );

  return true;
}


static bool
multicast_leave( struct vxlan_instance *instance ) {
  assert( instance != NULL );

  if ( !instance->multicast_joined ) {
    return false;
  }

  bool ret = ipv4_multicast_leave( instance->addr.sin_addr );
  if ( ret ) {
    instance->multicast_joined = false;
  }

  return ret;
}


bool
set_vxlan_instance_flooding_addr( uint8_t *vni, struct in_addr addr ) {
  assert( vxlan != NULL );
  assert( vni != NULL );

  struct vxlan_instance *instance = search_hash( &vxlan->instances, vni );
  if ( instance == NULL ) {
    return false;
  }

  if ( instance->addr.sin_addr.s_addr == addr.s_addr ) {
    return true;
  }

  if ( IN_MULTICAST( ntohl( instance->addr.sin_addr.s_addr ) ) ) {
    multicast_leave( instance );
  }

  if ( ntohl( addr.s_addr ) != INADDR_ANY ) {
    instance->addr.sin_addr = addr;
  }
  else {
    instance->addr.sin_addr = vxlan->flooding_addr;
  }

  if ( IN_MULTICAST( ntohl( instance->addr.sin_addr.s_addr ) ) ) {
    instance->multicast_joined = false;
  }
  else {
    instance->multicast_joined = true;
  }

  return multicast_join( instance );
}


bool
set_vxlan_instance_port( uint8_t *vni, uint16_t port ) {
  assert( vxlan != NULL );
  assert( vni != NULL );

  struct vxlan_instance *instance = search_hash( &vxlan->instances, vni );
  if ( instance == NULL ) {
    return false;
  }

  if ( port > 0 ) {
    instance->port = port;
  }
  else {
    instance->port = vxlan->flooding_port;
  }

  instance->addr.sin_port = htons( instance->port );

  return true;
}


bool
set_vxlan_instance_aging_time( uint8_t *vni, time_t aging_time ) {
  assert( vxlan != NULL );
  assert( vni != NULL );

  struct vxlan_instance *instance = search_hash( &vxlan->instances, vni );
  if ( instance == NULL ) {
    return false;
  }

  if ( aging_time >= 0 && aging_time <= VXLAN_MAX_AGING_TIME ) {
    instance->aging_time = aging_time;
  }
  else {
    instance->aging_time = vxlan->aging_time;
  }

  return set_aging_time( instance->fdb, instance->aging_time );
}


bool
inactivate_vxlan_instance( uint8_t *vni ) {
  assert( vxlan != NULL );
  assert( vni != NULL );

  struct vxlan_instance *instance = search_hash( &vxlan->instances, vni );
  if ( instance == NULL ) {
    return false;
  }

  if ( !instance->activated ) {
    return false;
  }

  instance->activated = false;
  tap_down( instance->vxlan_tap_name );

  return true;
}


bool
activate_vxlan_instance( uint8_t *vni ) {
  assert( vxlan != NULL );
  assert( vni != NULL );

  struct vxlan_instance *instance = search_hash( &vxlan->instances, vni );
  if ( instance == NULL ) {
    return false;
  }

  if ( instance->activated ) {
    return false;
  }

  instance->activated = true;
  tap_up( instance->vxlan_tap_name );

  return true;
}


bool
destroy_vxlan_instance( struct vxlan_instance *instance ) {
  assert( vxlan != NULL );
  assert( instance != NULL );

  if ( vxlan->n_instances <= 0 ) {
    return false;
  }

  int errors = 0;
  if ( IN_MULTICAST( ntohl( instance->addr.sin_addr.s_addr ) ) ) {
    if ( !multicast_leave( instance ) ) {
      warn( "Failed to leave from a multicast group." );
      errors++;
    }
  }

  tap_down( instance->vxlan_tap_name );
  instance->activated = false;

  int ret = -1;
  void *retval = NULL;

  if ( pthread_tryjoin_np( instance->tid, &retval ) == EBUSY ) {
    ret = pthread_cancel( instance->tid );
    if ( ret != 0 ) {
      warn( "Failed to terminate a vxlan instance ( ret = %d, tid = %u, tap = %s ).",
            ret, instance->tid, instance->vxlan_tap_name );
      errors++;
    }
    while ( pthread_tryjoin_np( instance->tid, &retval ) == EBUSY ) {
      struct timespec req = { 0, 50000000 };
      nanosleep( &req, NULL );
    }
  }

  ret = close( instance->tap_sock );
  if ( ret < 0 ) {
    char buf[ 256 ];
    char *error_string = safe_strerror_r( errno, buf, sizeof( buf ) );
    warn( "Failed to close tap socket ( ret = %d, instance = %s, errno = %s [%d] ).",
          ret, instance->vxlan_tap_name, error_string, errno );
    errors++;
  }
  destroy_fdb( instance->fdb );
  free( instance->fdb );

  vxlan->n_instances--;
  delete_hash( &vxlan->instances, instance->vni );
  free( instance );

  return errors == 0 ? true : false;
}


struct vxlan_instance **
get_all_vxlan_instances( int *n_instances ) {
  assert( vxlan != NULL );

  return ( struct vxlan_instance ** ) create_list_from_hash( &vxlan->instances, n_instances );
}


static bool
destroy_all_vxlan_instances() {
  assert( vxlan != NULL );
  
  int n_instances = 0;
  struct vxlan_instance **instances = get_all_vxlan_instances( &n_instances ); 
  for ( int n = 0; n < n_instances; n++ ) {
    destroy_vxlan_instance( instances[ n ] );
  }
  if ( instances != NULL ) {
    free( instances );
  }
  destroy_hash( &vxlan->instances );

  return true;
}


void
process_fdb_etherframe_from_vxlan( struct vxlan_instance *instance,
                                   struct ether_header *ether,
                                   struct sockaddr_in *vtep_addr ) {
  assert( vxlan != NULL );
  assert( instance != NULL );
  assert( ether != NULL );
  assert( vtep_addr != NULL );

  struct fdb_entry *entry = fdb_search_entry( instance->fdb, ( uint8_t * ) ether->ether_shost );
  if ( entry == NULL ) {
    vtep_addr->sin_port = htons( instance->port );
    fdb_add_entry( instance->fdb, ( uint8_t * ) ether->ether_shost, *vtep_addr );
  }
  else { 
    if ( entry->type == FDB_ENTRY_TYPE_DYNAMIC ) {
      if ( vtep_addr->sin_addr.s_addr != entry->vtep_addr.sin_addr.s_addr ) {
        entry->vtep_addr = *vtep_addr;
      }
      entry->ttl = instance->fdb->aging_time;
    }
  }
}


static void *
process_vxlan_instance( void *param ) {
  assert( vxlan != NULL );
  assert( param != NULL );

  char buf[ VXLAN_PACKET_BUF_LEN ];
  memset( buf, 0, sizeof( buf ) );
  char error_buf[ 256 ];

  struct vxlan_instance *instance = ( struct vxlan_instance * ) param;

  instance->activated = true;
  tap_up( instance->vxlan_tap_name );

  int tfd = -1;
  int fd_max = instance->tap_sock;
  while ( running ) {
    fdb_collect_garbage( instance->fdb );

    fd_set fds;
    FD_ZERO( &fds );
    FD_SET( instance->tap_sock, &fds );

    if ( !instance->multicast_joined ) {
      if ( tfd < 0 ) {
        tfd = timerfd_create( CLOCK_MONOTONIC, TFD_NONBLOCK );
        if ( tfd < 0 ) {
          critical( "Failed to create timerfd." );
          break;
        }
        struct itimerspec timer;
        memset( &timer, 0, sizeof( struct itimerspec ) );
        timer.it_value.tv_sec = 5;
        timer.it_interval.tv_sec = 5;
        timerfd_settime( tfd, 0, &timer, 0 );

        if ( instance->tap_sock > tfd ) {
          fd_max = instance->tap_sock;
        }
        else {
          fd_max = tfd;
        }
      }
      FD_SET( tfd, &fds );
    }

    struct timespec timeout = { 1, 0 };

    int ret = pselect( fd_max + 1, &fds, NULL, NULL, &timeout, NULL );
    if ( ret < 0 ) {
      if ( errno == EINTR ) {
        continue;
      }
      char *error_string = safe_strerror_r( errno, error_buf, sizeof( error_buf ) );
      error( "Failed to select ( ret = %d, errno = %s [%d] ).", ret, error_string, errno );
      break;
    }
    else if ( ret == 0 ) {
      continue;
    }

    if ( !instance->multicast_joined && tfd >= 0 ) {
      if ( FD_ISSET( tfd, &fds ) ) {
        uint64_t timer_count = 0;
        read( tfd, &timer_count, sizeof( timer_count ) );
        multicast_join( instance );
        if ( instance->multicast_joined ) {
          close( tfd );
          tfd = -1;
          fd_max = instance->tap_sock;
        }
      }
    }

    if ( FD_ISSET( instance->tap_sock, &fds ) ) {
      ssize_t len = read( instance->tap_sock, buf, sizeof( buf ) );
      if ( len < 0 ) {
        char *error_string = safe_strerror_r( errno, error_buf, sizeof( error_buf ) );
        warn( "Failed to read data from a tap device ( fd = %d, len = %d, errno = %s [%d] ).",
              instance->tap_sock, len, error_string, errno );
        continue;
      }

#if DEBUG
      debug( "%d bytes received from a tap interface.", len );
#endif

      if ( !instance->activated || !vxlan->active ) {
        continue;
      }

      send_etherframe_from_local_to_vxlan( instance, ( struct ether_header * ) buf, ( size_t ) len );
    }
  }

  return NULL;
}


bool
start_vxlan_instance( struct vxlan_instance *instance ) {
  assert( vxlan != NULL );
  assert( instance != NULL );

  instance->fdb = init_fdb( instance->aging_time );
  assert( instance->fdb != NULL );

  bool ret = multicast_join( instance );
  if ( !ret ) {
    return false;
  }

  pthread_attr_t attr;
  pthread_attr_init( &attr );
  int retval = pthread_attr_setstacksize( &attr, 4 * 1024 * 1024 );
  if ( retval != 0 ) {
    critical( "Failed to set stack size for a VXLAN instance thread." );
    return false;
  }
  retval = pthread_create( &instance->tid, &attr, process_vxlan_instance, instance );
  if ( retval != 0 ) {
    error( "Failed to create a VXLAN instance thread ( errno = %d ).", errno );
    return false;
  }

  return true;
}


bool
init_vxlan_instances( struct vxlan *_vxlan ) {
  assert( _vxlan != NULL );

  vxlan = _vxlan;

  return true;
}


bool
finalize_vxlan_instances() {
  assert( vxlan != NULL );

  destroy_all_vxlan_instances();

  return true;
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
