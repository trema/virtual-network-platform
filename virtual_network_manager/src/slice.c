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
#include <inttypes.h>
#include <limits.h>
#include <linux/limits.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "db.h"
#include "ovs.h"
#include "queue.h"
#include "slice.h"
#include "switch.h"
#include "transaction_manager.h"
#include "overlay_network_manager.h"
#include "utilities.h"
#ifdef UNIT_TEST
#include "slice_unittest.h"
#endif


#define PORT_NAME_LENGTH 256


enum {
  SLICE_STATE_CONFIRMED,
  SLICE_STATE_PREPARING_TO_UPDATE,
  SLICE_STATE_READY_TO_UPDATE,
  SLICE_STATE_UPDATING,
  SLICE_STATE_UPDATE_FAILED,
  SLICE_STATE_PREPARING_TO_DESTROY,
  SLICE_STATE_READY_TO_DESTROY,
  SLICE_STATE_DESTROYING,
  SLICE_STATE_DESTROY_FAILED,
  SLICE_STATE_DESTROYED,
  SLICE_STATE_MAX,
};

enum {
  PORT_STATE_CONFIRMED,
  PORT_STATE_PREPARING_TO_UPDATE,
  PORT_STATE_READY_TO_UPDATE,
  PORT_STATE_UPDATING,
  PORT_STATE_UPDATE_FAILED,
  PORT_STATE_PREPARING_TO_DESTROY,
  PORT_STATE_READY_TO_DESTROY,
  PORT_STATE_DESTROYING,
  PORT_STATE_DESTROY_FAILED,
  PORT_STATE_DESTROYED,
  PORT_STATE_MAX,
};

enum {
  MAC_STATE_INSTALLED,
  MAC_STATE_READY_TO_INSTALL,
  MAC_STATE_INSTALLING,
  MAC_STATE_INSTALL_FAILED,
  MAC_STATE_READY_TO_DELETE,
  MAC_STATE_DELETING,
  MAC_STATE_DELETE_FAILED,
  MAC_STATE_DELETED,
  MAC_STATE_MAX,
};

enum {
  PORT_TYPE_CUSTOMER,
  PORT_TYPE_OVERLAY,
  PORT_TYPE_MAX,
};

enum {
  MAC_TYPE_LOCAL,
  MAC_TYPE_REMOTE,
  MAC_TYPE_MAX,
};

typedef struct {
  uint64_t datapath_id;
  buffer *message;
  succeeded_handler succeeded_callback;
  void *succeeded_user_data;
  failed_handler failed_callback;
  void *failed_user_data;
} openflow_transaction;

typedef struct {
  uint32_t slice_id;
  int n_transactions;
  int n_overlay_network_transactions;
  bool failed;
  hash_table *port_updates;
} ongoing_slice_update;

typedef struct {
  uint64_t datapath_id;
  int n_transactions;
  int n_overlay_network_transactions;
  bool flow_entry_installation_started;
  bool failed;
  ongoing_slice_update *slice_update;
  hash_table *mac_updates;
  queue *transactions;
} ongoing_port_update;

typedef struct {
  uint32_t port_id;
  int n_transactions;
  bool failed;
  ongoing_port_update *port_update;
  queue *transactions;
} ongoing_mac_update;


static const struct timespec SLICE_DB_CHECK_INTERVAL = { 1, 0 };
static const unsigned int N_SLICE_UPDATES_AT_ONCE = 32;
static const unsigned int N_SWITCHES_TO_UPDATE_AT_ONCE = 128;
static MYSQL *db = NULL;
static char hostname[ HOST_NAME_MAX + 1 ];
static const uint64_t MAC_ADDRESS_MASK = 0xffffffffffff;
static hash_table *slice_updates = NULL;


static void
dump_ongoing_slice_update( ongoing_slice_update *update ) {
  assert( update != NULL );

  debug( "Slice update ( %p ): slice_id = %#x, n_transactions = %d, n_overlay_network_transactions = %d, failed = %s, port_updates = %p ).",
         update, update->slice_id, update->n_transactions, update->n_overlay_network_transactions,
         update->failed ? "true" : "false", update->port_updates );
}


static void
dump_ongoing_port_update( ongoing_port_update *update ) {
  assert( update != NULL );

  debug( "Port update ( %p ): datapath_id = %#" PRIx64 ", n_transactiond = %d, n_overlay_network_transactions = %d, "
         "flow_entry_installation_started = %s, failed = %s, ongoing_slice_update = %p, mac_updates = %p, transactions = %p.",
         update, update->datapath_id, update->n_transactions, update->n_overlay_network_transactions,
         update->flow_entry_installation_started ? "true" : "false", update->failed ? "true" : "false",
         update->slice_update, update->mac_updates, update->transactions );
}


static void
dump_ongoing_mac_update( ongoing_mac_update *update ) {
  assert( update != NULL );

  debug( "MAC update ( %p ): port_id = %#x, n_transactions = %d, failed = %s, port_update = %p, transactions = %p.",
         update, update->port_id, update->n_transactions, update->failed ? "true" : "false", update->port_update, update->transactions );
}


static openflow_transaction *
alloc_openflow_transaction() {
  debug( "Allocating an OpenFlow transaction." );

  openflow_transaction *transaction = xmalloc( sizeof( openflow_transaction ) );
  memset( transaction, 0, sizeof( openflow_transaction ) );

  debug( "A transaction is allocated ( %p ).", transaction );

  return transaction;
}


static void
free_openflow_transaction( openflow_transaction *transaction ) {
  debug( "Freeing an OpenFlow transaction ( %p ).", transaction );

  assert( transaction != NULL );

  if ( transaction->message != NULL ) {
    free_buffer( transaction->message );
  }
  xfree( transaction );

  debug( "A transaction is deallocated ( %p ).", transaction );
}


static bool
add_openflow_transaction( queue *transactions, uint64_t datapath_id, buffer *message,
                          succeeded_handler succeeded_callback, void *succeeded_user_data,
                          failed_handler failed_callback, void *failed_user_data ) {
  debug( "Adding an OpenFlow transaction ( transactions = %p, datapath_id = %#" PRIx64 ", message = %p, "
         "succeeded_callback = %p, succeeded_user_data = %p, failed_callback = %p, failed_user_data = %p ).",
         transactions, datapath_id, message, succeeded_callback, succeeded_user_data, failed_callback, failed_user_data );

  assert( transactions != NULL );

  openflow_transaction *transaction = alloc_openflow_transaction();
  transaction->datapath_id = datapath_id;
  transaction->message = message;
  transaction->succeeded_callback = succeeded_callback;
  transaction->succeeded_user_data = succeeded_user_data;
  transaction->failed_callback = failed_callback;
  transaction->failed_user_data = failed_user_data;

  enqueue( transactions, transaction );

  debug( "An OpenFlow transaction is pushed into a queue ( transactions = %p, transaction = %p ).",
         transactions, transaction );

  return true;
}


static bool
execute_openflow_transactions( queue *transactions ) {
  debug( "Executing OpenFlow transactions ( transactions = %p ).", transactions );

  assert( transactions != NULL );

  while ( 1 ) {
    openflow_transaction *x = dequeue( transactions );
    if ( x == NULL ) {
      break;
    }
    bool ret = execute_transaction( x->datapath_id, x->message,
                                    x->succeeded_callback, x->succeeded_user_data,
                                    x->failed_callback, x->failed_user_data );
    if ( !ret ) {
      enqueue( transactions, x );
      break;
    }
    free_openflow_transaction( x );
  }

  return true;
}


static bool
flush_openflow_transactions( ongoing_port_update *update ) {
  debug( "Flushing OpenFlow transactions ( %p ).", update );

  assert( update != NULL );

  dump_ongoing_port_update( update );

  if ( update->transactions->length > 0 ) {
    bool ret = execute_openflow_transactions( update->transactions );
    if ( !ret ) {
      error( "Failed to flush OpenFlow transactions ( port_update = %p ).", update );
      return false;
    }
  }

  hash_entry *e = NULL;
  hash_iterator iter;
  init_hash_iterator( update->mac_updates, &iter );
  while ( ( e = iterate_hash_next( &iter ) ) != NULL ) {
    ongoing_mac_update *mac_update = e->value;
    if ( mac_update->transactions->length > 0 ) {
      dump_ongoing_mac_update( mac_update );
      bool ret = execute_openflow_transactions( mac_update->transactions );
      if ( !ret ) {
        error( "Failed to flush OpenFlow transactions ( mac_update = %p ).", mac_update );
        return false;
      }
    }
  }

  return true;
}


static void
mark_ongoing_slice_update_as_failed( ongoing_slice_update *update ) {
  debug( "Marking an ongoing slice update as failed ( update = %p ).", update );

  assert( update != NULL );

  dump_ongoing_slice_update( update );

  update->failed = true;
}


static void
mark_ongoing_port_update_as_failed( ongoing_port_update *update ) {
  debug( "Marking an ongoing port update as failed ( update = %p ).", update );

  assert( update != NULL );
  assert( update->slice_update != NULL );

  dump_ongoing_port_update( update );

  update->failed = true;
  mark_ongoing_slice_update_as_failed( update->slice_update );
}


static void
mark_ongoing_mac_update_as_failed( ongoing_mac_update *update ) {
  debug( "Marking an ongoing MAC update as failed ( update = %p ).", update );

  assert( update != NULL );
  assert( update->port_update != NULL );

  dump_ongoing_mac_update( update );

  update->failed = true;
  mark_ongoing_port_update_as_failed( update->port_update );
}


static void
increment_slice_transactions( ongoing_slice_update *update, int n ) {
  debug( "Incrementing # of transactions in an ongoing slice update ( n = %d, update = %p ).", n, update );

  assert( update != NULL );

  dump_ongoing_slice_update( update );

  update->n_transactions += n;
}


static void
increment_overlay_network_transactions( ongoing_port_update *update, int n ) {
  debug( "Incrementing # of overlay network transactions in an ongoing port update ( n = %d, update = %p ).", n, update );

  assert( update != NULL );
  assert( update->slice_update != NULL );

  dump_ongoing_port_update( update );

  update->n_overlay_network_transactions += n;
  update->slice_update->n_overlay_network_transactions += n;
}


static void
increment_port_transactions( ongoing_port_update *update, int n ) {
  debug( "Incrementing # of transactions in an ongoing port update ( n = %d, update = %p ).", n, update );

  assert( update != NULL );
  assert( update->slice_update != NULL );

  dump_ongoing_port_update( update );

  update->n_transactions += n;
  increment_slice_transactions( update->slice_update, n );
}


static void
increment_mac_transactions( ongoing_mac_update *update, int n ) {
  debug( "Incrementing # of transactions in an ongoing MAC update ( n = %d, update = %p ).", n, update );

  assert( update != NULL );
  assert( update->port_update != NULL );

  dump_ongoing_mac_update( update );

  update->n_transactions += n;
  increment_port_transactions( update->port_update, n );
}


static void
decrement_slice_transactions( ongoing_slice_update *update, int n ) {
  debug( "Decrementing # of transactions in an ongoing slice update ( n = %d, update = %p ).", n, update );

  assert( update != NULL );
  assert( update->n_transactions >= n );

  dump_ongoing_slice_update( update );

  update->n_transactions -= n;
}


static void
decrement_overlay_network_transactions( ongoing_port_update *update, int n ) {
  debug( "Decrementing # of overlay network transactions in an ongoing port update ( n = %d, update = %p ).", n, update );

  assert( update != NULL );
  assert( update->n_overlay_network_transactions >= n );
  assert( update->slice_update != NULL );
  assert( update->slice_update->n_overlay_network_transactions >= n );

  dump_ongoing_port_update( update );

  update->n_overlay_network_transactions -= n;
  update->slice_update->n_overlay_network_transactions -= n;
}


static void
decrement_port_transactions( ongoing_port_update *update, int n ) {
  debug( "Decrementing # of transactions in an ongoing port update ( n = %d, update = %p ).", n, update );

  assert( update != NULL );
  assert( update->n_transactions >= n );
  assert( update->slice_update != NULL );

  dump_ongoing_port_update( update );

  update->n_transactions -= n;
  decrement_slice_transactions( update->slice_update, n );
}


static void
decrement_mac_transactions( ongoing_mac_update *update, int n ) {
  debug( "Decrementing # of transactions in an ongoing MAC update ( n = %d, update = %p ).", n, update );

  assert( update != NULL );
  assert( update->n_transactions >= n );
  assert( update->port_update != NULL );

  dump_ongoing_mac_update( update );

  update->n_transactions -= n;
  decrement_port_transactions( update->port_update, n );
}


static bool
compare_ongoing_mac_update( const void *x, const void *y ) {
  assert( x != NULL );
  assert( y != NULL );

  return ( *( const uint32_t * ) x == *( const uint32_t * ) y ) ? true : false;
}


static unsigned int
hash_ongoing_mac_update( const void *key ) {
  assert( key != NULL );

  return ( unsigned int ) *( const uint32_t * ) key;
}


static ongoing_mac_update *
add_ongoing_mac_update( ongoing_port_update *port_update, uint32_t port_id ) {
  debug( "Adding an ongoing MAC update ( port_id = %#x, port_update = %p ).", port_id, port_update );

  assert( port_update != NULL );
  assert( port_update->mac_updates != NULL );
  assert( port_update->slice_update != NULL );

  dump_ongoing_port_update( port_update );

  ongoing_mac_update *update = xmalloc( sizeof( ongoing_mac_update ) );
  memset( update, 0, sizeof( ongoing_mac_update ) );
  update->port_id = port_id;
  update->n_transactions = 0;
  update->failed = false;
  update->port_update = port_update;
  update->transactions = create_queue();

  void *duplicated = insert_hash_entry( port_update->mac_updates, &update->port_id, update );
  assert( duplicated == NULL );

  debug( "An ongoing MAC update is added ( %p ).", update );

  return update;
}


static void
delete_ongoing_mac_update( ongoing_port_update *port_update, uint32_t port_id ) {
  debug( "Deleting an ongoing MAC update ( port_id = %#x, port_update = %p ).", port_id, port_update );

  assert( port_update != NULL );
  assert( port_update->mac_updates != NULL );
  assert( port_update->slice_update != NULL );

  dump_ongoing_port_update( port_update );

  ongoing_mac_update *deleted = delete_hash_entry( port_update->mac_updates, &port_id );
  if ( deleted != NULL ) {
    dump_ongoing_mac_update( deleted );
    if ( deleted->failed ) {
      mark_ongoing_port_update_as_failed( port_update );
    }
    decrement_port_transactions( port_update, deleted->n_transactions );

    while ( 1 ) {
      openflow_transaction *x = dequeue( deleted->transactions );
      if ( x != NULL ) {
        free_openflow_transaction( x );
      }
      else {
        break;
      }
    }
    delete_queue( deleted->transactions );

    xfree( deleted );

    debug( "An ongoing MAC update is deleted ( port_id = %#x, port_update = %p ).", port_id, port_update );
  }
  else {
    warn( "Failed delete mac update record ( port_update = %p, datapath_id = %#" PRIx64 ", port_id = %#x ).",
          port_update, port_update->datapath_id, port_id );
  }
}


static bool
compare_ongoing_port_update( const void *x, const void *y ) {
  assert( x != NULL );
  assert( y != NULL );

  return ( *( const uint64_t * ) x == *( const uint64_t * ) y ) ? true : false;
}


static unsigned int
hash_ongoing_port_update( const void *key ) {
  assert( key != NULL );

  return ( unsigned int ) *( const uint64_t * ) key;
}


static bool
compare_ongoing_slice_update( const void *x, const void *y ) {
  assert( x != NULL );
  assert( y != NULL );

  return ( *( const uint32_t * ) x == *( const uint32_t * ) y ) ? true : false;
}


static unsigned int
hash_ongoing_slice_update( const void *key ) {
  assert( key != NULL );

  return ( unsigned int ) *( const uint32_t * ) key;
}


static ongoing_slice_update *
alloc_ongoing_slice_update( uint32_t slice_id ) {
  debug( "Allocating an ongoing slice_update ( slice_id = %#x ).", slice_id );

  ongoing_slice_update *update = xmalloc( sizeof( ongoing_slice_update ) );
  memset( update, 0, sizeof( ongoing_slice_update ) );
  update->slice_id = slice_id;
  update->n_transactions = 0;
  update->n_overlay_network_transactions = 0;
  update->failed = false;
  update->port_updates = create_hash_with_size( compare_ongoing_port_update, hash_ongoing_port_update, 128 );

  debug( "An ongoing slice update is allocated ( %p ).", update );

  return update;
}


static ongoing_slice_update *
add_ongoing_slice_update( uint32_t slice_id ) {
  debug( "Adding an ongoing slice update ( slice_id = %#x ).", slice_id );

  assert( slice_updates != NULL );

  ongoing_slice_update *update = lookup_hash_entry( slice_updates, &slice_id );
  if ( update != NULL ) {
    debug( "Duplicated ongoing slice update found ( %p ).", update );
    return update;
  }

  update = alloc_ongoing_slice_update( slice_id );
  assert( update != NULL );

  void *duplicated = insert_hash_entry( slice_updates, &update->slice_id, update );
  assert( duplicated == NULL );

  debug( "An ongoing slice update is added ( %p ).", update );

  return update;
}


static ongoing_port_update *
add_ongoing_port_update( ongoing_slice_update *slice_update, uint64_t datapath_id ) {
  debug( "Adding an ongoing port update ( slice_update = %p, datapath_id = %#" PRIx64 " ).", slice_update, datapath_id );

  assert( slice_update != NULL );
  assert( slice_update->port_updates != NULL );

  ongoing_port_update *update = lookup_hash_entry( slice_update->port_updates, &datapath_id );
  if ( update != NULL ) {
    debug( "Duplicated ongoing port update found ( %p ).", update );
    return update;
  }

  update = xmalloc( sizeof( ongoing_port_update ) );
  memset( update, 0, sizeof( ongoing_port_update ) );
  update->datapath_id = datapath_id;
  update->n_transactions = 0;
  update->n_overlay_network_transactions = 0;
  update->flow_entry_installation_started = false;
  update->failed = false;
  update->slice_update = slice_update;
  update->mac_updates = create_hash_with_size( compare_ongoing_mac_update, hash_ongoing_mac_update, 128 );
  update->transactions = create_queue();

  void *duplicated = insert_hash_entry( slice_update->port_updates, &update->datapath_id, update );
  assert( duplicated == NULL );

  debug( "An ongoing port update is added ( %p ).", update );

  return update;
}


static void
delete_ongoing_port_update( ongoing_slice_update *slice_update, uint64_t datapath_id ) {
  debug( "Deleting an ongoing port update ( slice_update = %p, datapath_id = %#" PRIx64 " ).", slice_update, datapath_id );

  assert( slice_update != NULL );
  assert( slice_update->port_updates != NULL );

  ongoing_port_update *deleted = delete_hash_entry( slice_update->port_updates, &datapath_id );
  if ( deleted != NULL ) {
    dump_ongoing_port_update( deleted );
    if ( deleted->failed ) {
      mark_ongoing_slice_update_as_failed( slice_update );
    }
    if ( deleted->mac_updates != NULL ) {
      hash_entry *e = NULL;
      hash_iterator iter;
      init_hash_iterator( deleted->mac_updates, &iter );
      while ( ( e = iterate_hash_next( &iter ) ) != NULL ) {
        ongoing_mac_update *mac_update = e->value;
        if ( mac_update != NULL ) {
          delete_ongoing_mac_update( deleted, mac_update->port_id );
        }
      }
      delete_hash( deleted->mac_updates );
    }
    decrement_slice_transactions( slice_update, deleted->n_transactions );

    while ( 1 ) {
      openflow_transaction *x = dequeue( deleted->transactions );
      if ( x != NULL ) {
        free_openflow_transaction( x );
      }
      else {
        break;
      }
    }
    delete_queue( deleted->transactions );

    xfree( deleted );

    debug( "An ongoing port update is deleted ( slice_update = %p, datapath_id = %#" PRIx64 " ).", slice_update, datapath_id );    
  }
  else {
    warn( "Failed delete port update record ( slice_update = %p, slice_id = %#x, datapath_id = %#" PRIx64 " ).",
          slice_update, slice_update->slice_id, datapath_id );
  }
}


static void
free_ongoing_slice_update( ongoing_slice_update *update ) {
  debug( "Freeing an ongoing slice update ( %p ).", update );

  assert( update != NULL );
  assert( update->port_updates != NULL );

  dump_ongoing_slice_update( update );

  hash_entry *e = NULL;
  hash_iterator iter;
  init_hash_iterator( update->port_updates, &iter );
  while ( ( e = iterate_hash_next( &iter ) ) != NULL ) {
    ongoing_port_update *port_update = e->value;
    if ( port_update != NULL ) {
      delete_ongoing_port_update( update, port_update->datapath_id );
    }
  }
  delete_hash( update->port_updates );

  xfree( update );

  debug( "An ongoing slice update is deleted ( %p ).", update );
}


static void
delete_ongoing_slice_update( uint32_t slice_id ) {
  debug( "Deleting an ongoing slice update ( slice_id = %#x ).", slice_id );

  assert( slice_updates != NULL );

  ongoing_slice_update *deleted = delete_hash_entry( slice_updates, &slice_id );
  if ( deleted != NULL ) {
    free_ongoing_slice_update( deleted );
    debug( "An ongoing slice update is deleted ( slice_id = %#x ).", slice_id );
  }
  else {
    warn( "Failed to delete an ongoing slice update ( slice_id = %#x ).", slice_id );
  }
}


static bool
get_slices_to_update( list_element **slices_to_update, int *n_slices ) {
  debug( "Retrieving the list of slices to be updated ( slices_to_update = %p, n_slices = %p ).", slices_to_update, n_slices );

  assert( db != NULL );
  assert( slices_to_update != NULL );
  assert( n_slices != NULL );

  *n_slices = 0;

  bool ret = execute_query( db, "select id,state from slices where "
                            "state = %u or state = %u or state = %u or state = %u limit %u",
                            SLICE_STATE_READY_TO_UPDATE, SLICE_STATE_READY_TO_DESTROY,
                            SLICE_STATE_UPDATING, SLICE_STATE_DESTROYING,
                            N_SLICE_UPDATES_AT_ONCE );
  if ( !ret ) {
    return false;
  }

  MYSQL_RES *result = mysql_store_result( db );
  if ( result == NULL ) {
    error( "Failed to retrieve result from database ( %s ).", mysql_error( db ) );
    return false;
  }

  assert( mysql_num_fields( result ) == 2 );

  if ( mysql_num_rows( result ) == 0 ) {
    mysql_free_result( result );
    return true;
  }

  create_list( slices_to_update );

  MYSQL_ROW row;
  while ( ( row = mysql_fetch_row( result ) ) != NULL ) {
    uint32_t *id = xmalloc( sizeof( uint32_t ) );
    char *endp = NULL;
    *id = ( uint32_t ) strtoul( row[ 0 ], &endp, 0 );
    if ( *endp != '\0' ) {
      error( "Invalid slice_id ( %s ).", row[ 0 ] );
      xfree( id );
      continue;
    }
    debug( "Slice %#x needs to be updated.", *id );
    append_to_tail( slices_to_update, id );
    ( *n_slices )++;
  }

  if ( *n_slices == 0 ) {
    delete_list( *slices_to_update );
  }

  mysql_free_result( result );

  debug( "%d slices need to be updated.", *n_slices );

  return true;
}


static bool
get_switches_to_update( uint32_t slice_id, list_element **switches_to_update, int *n_switches ) {
  debug( "Retrieving the list of switches to be updated ( slice_id = %#x, switches_to_update = %p, n_switches = %p ).",
         slice_id, switches_to_update, n_switches );

  assert( db != NULL );
  assert( switches_to_update != NULL );
  assert( n_switches != NULL );

  *n_switches = 0;

  bool ret = execute_query( db, "select distinct(datapath_id) from ports where slice_id = %u and "
                            "( state = %u or state = %u or state = %u or state = %u ) limit %u",
                            slice_id, PORT_STATE_READY_TO_UPDATE, PORT_STATE_READY_TO_DESTROY,
                            PORT_STATE_UPDATING, PORT_STATE_DESTROYING,
                            N_SWITCHES_TO_UPDATE_AT_ONCE );
  if ( !ret ) {
    return false;
  }

  MYSQL_RES *result = mysql_store_result( db );
  if ( result == NULL ) {
    error( "Failed to retrieve result from database ( %s ).", mysql_error( db ) );
    return false;
  }

  assert( mysql_num_fields( result ) == 1 );

  if ( mysql_num_rows( result ) == 0 ) {
    mysql_free_result( result );
    return true;
  }

  create_list( switches_to_update );

  pid_t pid = getpid();
  MYSQL_ROW row;
  while ( ( row = mysql_fetch_row( result ) ) != NULL ) {
    char *endp = NULL;
    uint64_t datapath_id = ( uint64_t ) strtoull( row[ 0 ], &endp, 0 );
    if ( *endp != '\0' ) {
      error( "Invalid datapath_id ( %s ).", row[ 0 ] );
      continue;
    }

    if ( !switch_on_duty( datapath_id, hostname, pid ) ) {
      continue;
    }

    uint64_t *datapath_id_to_add = xmalloc( sizeof( uint64_t ) );
    *datapath_id_to_add = datapath_id;
    append_to_tail( switches_to_update, datapath_id_to_add );
    ( *n_switches )++;

    debug( "Switch %#" PRIx64 " needs to be updated.", *datapath_id_to_add );
  }

  if ( *n_switches == 0 ) {
    delete_list( *switches_to_update );
  }

  mysql_free_result( result );

  debug( "%d switches need to be updated.", *n_switches );

  return true;
}


typedef struct {
  uint64_t datapath_id;
  uint32_t port_id;
  uint16_t port_no;
  char port_name[ PORT_NAME_LENGTH ];
  uint16_t vid;
  uint8_t type;
} port_in_slice;


static void
dump_port_in_slice( port_in_slice *port ) {
  assert( port != NULL );

  debug( "Port in slice ( %p ): datapath_id = %#" PRIx64 ", port_id = %#x, port_no = %u, port_name = %s, vid = %#x, type = %#x.",
         port, port->datapath_id, port->port_id, port->port_no, port->port_name, port->vid, port->type );
}


static bool
append_port( MYSQL_ROW row, const uint64_t datapath_id, list_element **ports ) {
  debug( "Appending a port to a list ( row = %p, datapath_id = %#" PRIx64 ", ports = %p ).", row, datapath_id, ports );

  assert( row != NULL );
  assert( ports != NULL );

  port_in_slice *port = xmalloc( sizeof( port_in_slice ) );
  memset( port, 0, sizeof( port_in_slice ) );

  port->datapath_id = datapath_id;

  char *endp = NULL;
  port->port_id = ( uint32_t ) strtoul( row[ 0 ], &endp, 0 );
  if ( *endp != '\0' ) {
    error( "Invalid port id ( %s ).", row[ 0 ] );
    xfree( port );
    return false;
  }

  uint32_t u32 = ( uint32_t ) strtoul( row[ 1 ], &endp, 0 );
  if ( *endp != '\0' || u32 > UINT16_MAX ) {
    error( "Invalid port number ( %s ).", row[ 1 ] );
    xfree( port );
    return false;
  }
  port->port_no = ( uint16_t ) u32;

  strncpy( port->port_name, row[ 2 ], sizeof( port->port_name ) - 1 );

  u32 = ( uint32_t ) strtoul( row[ 3 ], &endp, 0 );
  if ( *endp != '\0' || u32 > UINT16_MAX ) {
    error( "Invalid vlan id ( %s ).", row[ 3 ] );
    xfree( port );
    return false;
  }
  port->vid = ( uint16_t ) u32;

  u32 = ( uint32_t ) strtoul( row[ 4 ], &endp, 0 );
  if ( *endp != '\0' || u32 >= PORT_TYPE_MAX ) {
    error( "Invalid type ( %s ).", row[ 4 ] );
    xfree( port );
    return false;
  }
  port->type = ( uint8_t ) u32;

  if ( port->port_no == OFPP_NONE ) {
    if ( strlen( port->port_name ) > 0 ) {
      bool ret = get_port_no_by_name( datapath_id, port->port_name, &port->port_no );
      if ( !ret ) {
        error( "Failed to retrieve port number from name ( datapath_id = %#" PRIx64 ", name = %s ).",
               datapath_id, port->port_name );
        return false;
      }
    }
    else {
      error( "Either port number or port name must be specified." );
      return false;
    }
  }

  append_to_tail( ports, port );

  debug( "Port %s ( port_id = %#x, port_no = %u, vid = %#x ) is appended to a list ( %p ).",
         port->port_name, port->port_id, port->port_no, port->vid, ports );

  return true;
}


static bool
get_active_ports_in_slice( uint64_t datapath_id, uint32_t slice_id, list_element **ports_in_slice, int *n_ports ) {
  debug( "Retrieving the list active/activating ports in a slice ( slice_id = %#x, datapath_id = %#" PRIx64 ", "
         "ports_in_slice = %p, n_ports = %p ).", slice_id, datapath_id, ports_in_slice, n_ports );

  assert( db != NULL );
  assert( ports_in_slice != NULL );
  assert( n_ports != NULL );

  *n_ports = 0;

  bool ret = execute_query( db, "select id,port_no,port_name,vid,type,state from ports where datapath_id = %" PRIu64 " and "
                            "slice_id = %u and ( state = %u or state = %u or state = %u )",
                            datapath_id, slice_id,
                            PORT_STATE_CONFIRMED, PORT_STATE_READY_TO_UPDATE, PORT_STATE_UPDATING );
  if ( !ret ) {
    return false;
  }

  MYSQL_RES *result = mysql_store_result( db );
  if ( result == NULL ) {
    error( "Failed to retrieve result from database ( %s ).", mysql_error( db ) );
    return false;
  }

  assert( mysql_num_fields( result ) == 6 );

  if ( mysql_num_rows( result ) == 0 ) {
    mysql_free_result( result );
    return true;
  }

  create_list( ports_in_slice );

  int errors = 0;
  MYSQL_ROW row;
  while ( ( row = mysql_fetch_row( result ) ) != NULL ) {
    bool ret = append_port( row, datapath_id, ports_in_slice );
    if ( ret ) {
      ( *n_ports )++;
    }
    else {
      errors++;
    }
  }

  if ( *n_ports == 0 ) {
    delete_list( *ports_in_slice );
  }

  mysql_free_result( result );

  debug( "%d active/activating ports found.", *n_ports );

  return errors == 0 ? true : false;
}


static bool
get_inactive_ports_in_slice( uint64_t datapath_id, uint32_t slice_id, list_element **ports_in_slice, int *n_ports ) {
  debug( "Retrieving the list inactive/inactivating ports in a slice ( slice_id = %#x, datapath_id = %#" PRIx64 ", "
         "ports_in_slice = %p, n_ports = %p ).", slice_id, datapath_id, ports_in_slice, n_ports );

  assert( db != NULL );
  assert( ports_in_slice != NULL );
  assert( n_ports != NULL );

  *n_ports = 0;

  bool ret = execute_query( db, "select id,port_no,port_name,vid,type,state from ports where datapath_id = %" PRIu64 " and "
                            "slice_id = %u and ( state = %u or state = %u or state = %u )",
                            datapath_id, slice_id,
                            PORT_STATE_READY_TO_DESTROY, PORT_STATE_DESTROYING, PORT_STATE_DESTROYED );
  if ( !ret ) {
    return false;
  }

  MYSQL_RES *result = mysql_store_result( db );
  if ( result == NULL ) {
    error( "Failed to retrieve result from database ( %s ).", mysql_error( db ) );
    return false;
  }

  assert( mysql_num_fields( result ) == 6 );

  if ( mysql_num_rows( result ) == 0 ) {
    mysql_free_result( result );
    return true;
  }

  create_list( ports_in_slice );

  int errors = 0;
  MYSQL_ROW row;
  while ( ( row = mysql_fetch_row( result ) ) != NULL ) {
    bool ret = append_port( row, datapath_id, ports_in_slice );
    if ( ret ) {
      ( *n_ports )++;
    }
    else {
      errors++;
    }
  }

  if ( *n_ports == 0 ) {
    delete_list( *ports_in_slice );
  }

  mysql_free_result( result );

  debug( "%d inactive/inactivating ports found.", *n_ports );

  return errors == 0 ? true : false;
}


static bool
append_overlay_port( MYSQL_ROW row, list_element **ports, pid_t pid ) {
  debug( "Appending an overlay port to a list ( row = %p, ports = %p, pid = %u ).", row, ports, pid );

  assert( row != NULL );
  assert( ports != NULL );
  assert( pid > 0 );

  char *endp = NULL;
  uint64_t datapath_id = ( uint64_t ) strtoul( row[ 1 ], &endp, 0 );
  if ( *endp != '\0' ) {
    error( "Invalid datapath id ( %s ).", row[ 1 ] );
    return false;
  }

  if ( !switch_on_duty( datapath_id, hostname, pid ) ) {
    return false;
  }

  port_in_slice *port = xmalloc( sizeof( port_in_slice ) );
  memset( port, 0, sizeof( port_in_slice ) );

  port->port_id = ( uint32_t ) strtoul( row[ 0 ], &endp, 0 );
  if ( *endp != '\0' ) {
    error( "Invalid port id ( %s ).", row[ 0 ] );
    xfree( port );
    return false;
  }

  port->datapath_id = datapath_id;
  port->port_no = UINT16_MAX;
  strncpy( port->port_name, row[ 2 ], sizeof( port->port_name ) - 1 );

  uint32_t u32 = ( uint32_t ) strtoul( row[ 3 ], &endp, 0 );
  if ( *endp != '\0' || u32 > UINT16_MAX ) {
    error( "Invalid vlan id ( %s ).", row[ 3 ] );
    xfree( port );
    return false;
  }
  port->vid = ( uint16_t ) u32;

  u32 = ( uint32_t ) strtoul( row[ 4 ], &endp, 0 );
  if ( *endp != '\0' || u32 >= PORT_TYPE_MAX ) {
    error( "Invalid type ( %s ).", row[ 4 ] );
    xfree( port );
    return false;
  }
  port->type = ( uint8_t ) u32;

  append_to_tail( ports, port );

  debug( "port %s ( port_id = %#x, port_no = %u, vid = %#x, type = %#x ) is appended to port list ( %p ).",
         port->port_name, port->port_id, port->port_no, port->vid, port->type, ports );

  return true;
}


static bool
get_activating_overlay_port( uint32_t slice_id, uint64_t datapath_id, list_element **ports_in_slice, int *n_ports ) {
  debug( "Retrieving the list of active/activating overlay port ( slice_id = %#x, datapath_id = %#" PRIx64 ", "
         "ports_in_slice = %p, n_ports = %p ).", slice_id, datapath_id, ports_in_slice, n_ports );

  assert( db != NULL );
  assert( ports_in_slice != NULL );
  assert( n_ports != NULL );

  *n_ports = 0;

  bool ret = execute_query( db, "select id,datapath_id,port_name,vid,type from ports where slice_id = %u and datapath_id = %" PRIu64 " and "
                            "type = %u and ( state = %u or state = %u )",
                            slice_id, datapath_id, PORT_TYPE_OVERLAY, PORT_STATE_READY_TO_UPDATE, PORT_STATE_UPDATING );
  if ( !ret ) {
    return false;
  }

  MYSQL_RES *result = mysql_store_result( db );
  if ( result == NULL ) {
    error( "Failed to retrieve result from database ( %s ).", mysql_error( db ) );
    return false;
  }

  assert( mysql_num_fields( result ) == 5 );

  if ( mysql_num_rows( result ) == 0 ) {
    mysql_free_result( result );
    return true;
  }

  create_list( ports_in_slice );

  pid_t pid = getpid();
  MYSQL_ROW row;
  while ( ( row = mysql_fetch_row( result ) ) != NULL ) {
    bool ret = append_overlay_port( row, ports_in_slice, pid );
    if ( ret ) {
      ( *n_ports )++;
    }
  }

  if ( *n_ports == 0 ) {
    delete_list( *ports_in_slice );
  }

  mysql_free_result( result );

  debug( "%d active/activating overlay ports found.", *n_ports );

  return true;
}


static bool
get_inactivating_overlay_port( uint32_t slice_id, uint64_t datapath_id, list_element **ports_in_slice, int *n_ports ) {
  debug( "Retrieving the list of inactive/inactivating overlay port ( slice_id = %#x, datapath_id = %#" PRIx64 ", "
         "ports_in_slice = %p, n_ports = %p ).", slice_id, datapath_id, ports_in_slice, n_ports );

  assert( db != NULL );
  assert( ports_in_slice != NULL );
  assert( n_ports != NULL );

  *n_ports = 0;

  bool ret = execute_query( db, "select id,datapath_id,port_name,vid,type from ports where slice_id = %u and datapath_id = %" PRIu64 " and "
                            "type = %u and ( state = %u or state = %u )",
                            slice_id, datapath_id, PORT_TYPE_OVERLAY, PORT_STATE_READY_TO_DESTROY, PORT_STATE_DESTROYING );
  if ( !ret ) {
    return false;
  }

  MYSQL_RES *result = mysql_store_result( db );
  if ( result == NULL ) {
    error( "Failed to retrieve result from database ( %s ).", mysql_error( db ) );
    return false;
  }

  assert( mysql_num_fields( result ) == 5 );

  if ( mysql_num_rows( result ) == 0 ) {
    mysql_free_result( result );
    return true;
  }

  create_list( ports_in_slice );

  pid_t pid = getpid();
  MYSQL_ROW row;
  while ( ( row = mysql_fetch_row( result ) ) != NULL ) {
    bool ret = append_overlay_port( row, ports_in_slice, pid );
    if ( ret ) {
      ( *n_ports )++;
    }
  }

  if ( *n_ports == 0 ) {
    delete_list( *ports_in_slice );
  }

  mysql_free_result( result );

  debug( "%d inactive/inactivating overlay ports found.", *n_ports );

  return true;
}


static bool
get_mac_addresses( uint32_t slice_id, uint32_t port_id, list_element **mac_addresses, int *n_addresses, uint8_t state ) {
  debug( "Retrieving the list MAC addresses associated with a port ( slice_id = %#x, port_id = %#x, mac_addresses = %p, "
         "n_addresses = %p, state = %#x ).", slice_id, port_id, mac_addresses, n_addresses, state );

  assert( db != NULL );
  assert( mac_addresses != NULL );
  assert( n_addresses != NULL );
  assert( state < MAC_STATE_MAX );

  *n_addresses = 0;

  bool ret = execute_query( db, "select mac from mac_addresses where slice_id = %u and "
                            "port_id = %u and state = %u", slice_id, port_id, state );
  if ( !ret ) {
    return false;
  }

  MYSQL_RES *result = mysql_store_result( db );
  if ( result == NULL ) {
    error( "Failed to retrieve result from database ( %s ).", mysql_error( db ) );
    return false;
  }

  assert( mysql_num_fields( result ) == 1 );

  if ( mysql_num_rows( result ) == 0 ) {
    mysql_free_result( result );
    return true;
  }

  create_list( mac_addresses );

  MYSQL_ROW row;
  while ( ( row = mysql_fetch_row( result ) ) != NULL ) {
    char *endp = NULL;
    uint64_t mac = ( uint64_t ) strtoull( row[ 0 ], &endp, 0 );
    if ( *endp != '\0' ) {
      error( "Invalid MAC address ( %s ).", row[ 0 ] );
      continue;
    }
    uint64_t *mac_to_append = xmalloc( sizeof( uint64_t ) );
    *mac_to_append = mac;
    append_to_tail( mac_addresses, mac_to_append );
    ( *n_addresses )++;
  }

  if ( *n_addresses == 0 ) {
    delete_list( *mac_addresses );
  }

  mysql_free_result( result );

  debug( "%d MAC addresses found.", *n_addresses );

  return true;
}


static bool
get_mac_addresses_to_install( uint32_t slice_id, uint32_t port_id, list_element **mac_addresses, int *n_addresses ) {
  debug( "Retrieving the list of MAC addresses to be installed ( slice_id = %#x, port_id = %#x, "
         "mac_addresses = %p, n_addresses = %p ).", slice_id, port_id, mac_addresses, n_addresses );

  assert( db != NULL );
  assert( mac_addresses != NULL );
  assert( n_addresses != NULL );

  return get_mac_addresses( slice_id, port_id, mac_addresses, n_addresses, MAC_STATE_INSTALLING );
}


static bool
get_mac_addresses_to_delete( uint32_t slice_id, uint32_t port_id, list_element **mac_addresses, int *n_addresses ) {
  debug( "Retrieving the list of MAC addresses to be deleted ( slice_id = %#x, port_id = %#x, "
         "mac_addresses = %p, n_addresses = %p ).", slice_id, port_id, mac_addresses, n_addresses );

  assert( db != NULL );
  assert( mac_addresses != NULL );
  assert( n_addresses != NULL );

  return get_mac_addresses( slice_id, port_id, mac_addresses, n_addresses, MAC_STATE_DELETING );
}


static bool
valid_mac_state_transition( uint8_t from, uint8_t to ) {
  debug( "Checking MAC address state transaction ( from = %#x, to = %#x ).", from, to );

  assert( from < MAC_STATE_MAX );
  assert( to < MAC_STATE_MAX );

  switch ( from ) {
    case MAC_STATE_INSTALLED:
    {
      goto invalid_state_transition;
    }
    break;

    case MAC_STATE_READY_TO_INSTALL:
    {
      if ( to != MAC_STATE_INSTALLING ) {
        goto invalid_state_transition;
      }
    }
    break;

    case MAC_STATE_INSTALLING:
    {
      if ( to != MAC_STATE_INSTALLED && to != MAC_STATE_INSTALL_FAILED ) {
        goto invalid_state_transition;
      }
    }
    break;

    case MAC_STATE_INSTALL_FAILED:
    {
      goto invalid_state_transition;
    }
    break;

    case MAC_STATE_READY_TO_DELETE:
    {
      if ( to != MAC_STATE_DELETING ) {
        goto invalid_state_transition;
      }
    }
    break;

    case MAC_STATE_DELETING:
    {
      if ( to != MAC_STATE_DELETED && to != MAC_STATE_DELETE_FAILED ) {
        goto invalid_state_transition;
      }
    }
    break;

    case MAC_STATE_DELETE_FAILED:
    {
      goto invalid_state_transition;
    }
    break;

    case MAC_STATE_DELETED:
    {
      goto invalid_state_transition;
    }
    break;

    default:
    {
      goto invalid_state_transition;
    }
    break;
  }

  debug( "Valid MAC address state transaction ( from = %#x, to = %#x ).", from, to );

  return true;

invalid_state_transition:
  debug( "Invalid MAC address state transaction ( from = %#x, to = %#x ).", from, to );

  return false;
}


static bool
update_mac_states( uint32_t slice_id, uint32_t port_id, uint8_t from, uint8_t to ) {
  debug( "Updating MAC address states ( slice_id = %#x, port_id = %#x, from = %#x, to = %#x ).",
         slice_id, port_id, from, to );

  assert( db != NULL );
  assert( from < MAC_STATE_MAX );
  assert( to < MAC_STATE_MAX );

  if ( !valid_mac_state_transition( from, to ) ) {
    error( "Invalid MAC address state transition ( slice_id = %#x, port_id = %#x, from = %#x, to = %#x ).",
           slice_id, port_id, from, to );
    return false;
  }

  bool ret = execute_query( db, "update mac_addresses set state = %u where slice_id = %u and "
                            "port_id = %u and state = %u", to, slice_id, port_id, from );
  if ( !ret ) {
    return false;
  }

  my_ulonglong n_affected_rows = mysql_affected_rows( db );
  debug( "%" PRIu64 " entries are updated.", ( uint64_t ) n_affected_rows );

  return true;
}


static bool
delete_deleted_mac_addresses( uint32_t slice_id ) {
  debug( "Deleting deleted MAC addresses ( slice_id = %#x ).", slice_id );

  assert( db != NULL );

  bool ret = execute_query( db, "delete from mac_addresses where slice_id = %u and state = %u",
                            slice_id, MAC_STATE_DELETED );
  if ( !ret ) {
    return false;
  }

  my_ulonglong n_affected_rows = mysql_affected_rows( db );
  debug( "%" PRIu64 " entries are deleted.", ( uint64_t ) n_affected_rows );

  return true;
}


static bool
valid_port_state_transition( uint8_t from, uint8_t to ) {
  debug( "Checking port state transaction ( from = %#x, to = %#x ).", from, to );

  assert( from < PORT_STATE_MAX );
  assert( to < PORT_STATE_MAX );

  switch ( from ) {
    case PORT_STATE_CONFIRMED:
    {
      goto invalid_state_transition;
    }
    break;

    case PORT_STATE_PREPARING_TO_UPDATE:
    {
      goto invalid_state_transition;
    }
    break;

    case PORT_STATE_READY_TO_UPDATE:
    {
      if ( to != PORT_STATE_UPDATING ) {
        goto invalid_state_transition;
      }
    }
    break;

    case PORT_STATE_UPDATING:
    {
      if ( to != PORT_STATE_CONFIRMED && to != PORT_STATE_UPDATE_FAILED ) {
        goto invalid_state_transition;
      }
    }
    break;

    case PORT_STATE_UPDATE_FAILED:
    {
      goto invalid_state_transition;
    }
    break;

    case PORT_STATE_PREPARING_TO_DESTROY:
    {
      goto invalid_state_transition;
    }
    break;

    case PORT_STATE_READY_TO_DESTROY:
    {
      if ( to != PORT_STATE_DESTROYING ) {
        goto invalid_state_transition;
      }
    }
    break;

    case PORT_STATE_DESTROYING:
    {
      if ( to != PORT_STATE_DESTROYED && to != PORT_STATE_DESTROY_FAILED ) {
        goto invalid_state_transition;
      }
    }
    break;

    case PORT_STATE_DESTROY_FAILED:
    {
      goto invalid_state_transition;
    }
    break;

    case PORT_STATE_DESTROYED:
    {
      goto invalid_state_transition;
    }
    break;

    default:
    {
      goto invalid_state_transition;
    }
    break;
  }

  debug( "Valid port state transaction ( from = %#x, to = %#x ).", from, to );

  return true;

invalid_state_transition:
  debug( "Invalid port state transaction ( from = %#x, to = %#x ).", from, to );

  return false;
}


static bool
update_port_states( uint64_t datapath_id, uint32_t slice_id, uint8_t from, uint8_t to ) {
  debug( "Updating port states ( slice_id = %#x, datapath_id = %#" PRIx64 ", from = %#x, to = %#x ).",
         slice_id, datapath_id, from, to );

  assert( db != NULL );
  assert( from < PORT_STATE_MAX );
  assert( to < PORT_STATE_MAX );

  if ( !valid_port_state_transition( from, to ) ) {
    error( "Invalid port state transition ( slice_id = %#x, datapath_id = %#" PRIx64 ", from = %#x, to = %#x ).",
           slice_id, datapath_id, from, to );
    return false;
  }

  bool ret = execute_query( db, "update ports set state = %u where datapath_id = %" PRIu64
                            " and slice_id = %u and state = %u",
                            to, datapath_id, slice_id, from );
  if ( !ret ) {
    return false;
  }

  my_ulonglong n_affected_rows = mysql_affected_rows( db );
  debug( "%" PRIu64 " entries are updated.", ( uint64_t ) n_affected_rows );

  return true;
}


static bool
delete_destroyed_ports( uint32_t slice_id ) {
  debug( "Deleting destroyed ports ( slice_id = %#x ).", slice_id );

  assert( db != NULL );

  bool ret = execute_query( db, "delete from ports where slice_id = %u and state = %u",
                            slice_id, PORT_STATE_DESTROYED );
  if ( !ret ) {
    return false;
  }

  my_ulonglong n_affected_rows = mysql_affected_rows( db );
  debug( "%" PRIu64 " entries are deleted.", ( uint64_t ) n_affected_rows );

  return true;
}


static bool
valid_slice_state_transition( uint8_t from, uint8_t to ) {
  debug( "Checking slice state transaction ( from = %#x, to = %#x ).", from, to );

  assert( from < SLICE_STATE_MAX );
  assert( to < SLICE_STATE_MAX );

  switch ( from ) {
    case SLICE_STATE_CONFIRMED:
    {
      goto invalid_state_transition;
    }
    break;

    case SLICE_STATE_PREPARING_TO_UPDATE:
    {
      goto invalid_state_transition;
    }
    break;

    case SLICE_STATE_READY_TO_UPDATE:
    {
      if ( to != SLICE_STATE_UPDATING ) {
        goto invalid_state_transition;
      }
    }
    break;

    case SLICE_STATE_UPDATING:
    {
      if ( to != SLICE_STATE_CONFIRMED && to != SLICE_STATE_UPDATE_FAILED ) {
        goto invalid_state_transition;
      }
    }
    break;

    case SLICE_STATE_UPDATE_FAILED:
    {
      goto invalid_state_transition;
    }
    break;

    case SLICE_STATE_PREPARING_TO_DESTROY:
    {
      goto invalid_state_transition;
    }
    break;

    case SLICE_STATE_READY_TO_DESTROY:
    {
      if ( to != SLICE_STATE_DESTROYING ) {
        goto invalid_state_transition;
      }
    }
    break;

    case SLICE_STATE_DESTROYING:
    {
      if ( to != SLICE_STATE_DESTROYED && to != SLICE_STATE_DESTROY_FAILED ) {
        goto invalid_state_transition;
      }
    }
    break;

    case SLICE_STATE_DESTROY_FAILED:
    {
      goto invalid_state_transition;
    }
    break;

    case SLICE_STATE_DESTROYED:
    {
      goto invalid_state_transition;
    }
    break;

    default:
    {
      goto invalid_state_transition;
    }
    break;
  }

  debug( "Valid slice state transaction ( from = %#x, to = %#x ).", from, to );

  return true;

invalid_state_transition:
  debug( "Invalid slice state transaction ( from = %#x, to = %#x ).", from, to );

  return false;
}


static bool
update_slice_states( uint8_t from, uint8_t to ) {
  debug( "Updating slice states ( from = %#x, to = %#x ).", from, to );

  assert( db != NULL );
  assert( from < SLICE_STATE_MAX );
  assert( to < SLICE_STATE_MAX );

  if ( !valid_slice_state_transition( from, to ) ) {
    critical( "Invalid slice state transition ( %#x -> %#x ).", from, to );
    return false;
  }

  if ( to != SLICE_STATE_DESTROYED ) {
    bool ret = execute_query( db, "update slices set state = %u where state = %u", to, from );
    if ( !ret ) {
      return false;
    }
  }
  else {
    bool ret = execute_query( db, "delete from slices where state = %u", from );
    if ( !ret ) {
      return false;
    }
  }

  my_ulonglong n_affected_rows = mysql_affected_rows( db );
  debug( "%" PRIu64 " entries are updated.", ( uint64_t ) n_affected_rows );

  return true;
}


static void
update_slice_state_on_complete( uint32_t slice_id, bool failed ) {
  debug( "Updating slice state ( slice_id = %#x, failed = %s ).", slice_id, failed ? "true" : "false" );

  assert( db != NULL );

  bool ret = false;
  ret = execute_query( db, "update slices set state = %u where id = %u and state = %u and "
                       "( select count(*) from ports where slice_id = %u and "
                       "( state = %u or state = %u ) ) > 0 and "
                       "( select count(*) from ports where slice_id = %u and "
                       "( state = %u or state = %u or state = %u or state = %u ) ) = 0",
                       SLICE_STATE_UPDATE_FAILED, slice_id, SLICE_STATE_UPDATING,
                       slice_id, PORT_STATE_UPDATE_FAILED, PORT_STATE_DESTROY_FAILED,
                       slice_id, PORT_STATE_READY_TO_UPDATE, PORT_STATE_UPDATING,
                       PORT_STATE_READY_TO_DESTROY, PORT_STATE_DESTROYING );
  if ( !ret ) {
    return;
  }
  if ( mysql_affected_rows( db ) > 0 ) {
    info( "Slice %#x transitioned to UPDATE_FAILED by %s ( pid = %u ).", slice_id, hostname, getpid() );
    goto completed;
  }

  ret = execute_query( db, "update slices set state = %u where id = %u and state = %u and "
                       "( select count(*) from ports where slice_id = %u and "
                       "( state = %u or state = %u ) ) > 0 and "
                       "( select count(*) from ports where slice_id = %u and "
                       "( state = %u or state = %u or state = %u or state = %u ) ) = 0",
                       SLICE_STATE_DESTROY_FAILED, slice_id, SLICE_STATE_DESTROYING,
                       slice_id, PORT_STATE_UPDATE_FAILED, PORT_STATE_DESTROY_FAILED,
                       slice_id, PORT_STATE_READY_TO_UPDATE, PORT_STATE_UPDATING,
                       PORT_STATE_READY_TO_DESTROY, PORT_STATE_DESTROYING );
  if ( !ret ) {
    return;
  }
  if ( mysql_affected_rows( db ) > 0 ) {
    info( "Slice %#x transitioned to DESTROY_FAILED by %s ( pid = %u ).", slice_id, hostname, getpid() );
    goto completed;
  }

  if ( failed ) {
    debug( "Failed to update slice state ( slice_id = %#x, failed = %u ).", slice_id, failed );
    return;
  }

  ret = execute_query( db, "update slices set state = %u where id = %u and state = %u and "
                       "( select count(*) from ports where slice_id = %u and "
                       "( state = %u or state = %u ) ) = 0 and "
                       "( select count(*) from ports where slice_id = %u and "
                       "( state = %u or state = %u or state = %u or state = %u ) ) = 0",
                       SLICE_STATE_CONFIRMED, slice_id, SLICE_STATE_UPDATING,
                       slice_id, PORT_STATE_UPDATE_FAILED, PORT_STATE_DESTROY_FAILED,
                       slice_id, PORT_STATE_READY_TO_UPDATE, PORT_STATE_UPDATING,
                       PORT_STATE_READY_TO_DESTROY, PORT_STATE_DESTROYING );
  if ( !ret ) {
    return;
  }
  if ( mysql_affected_rows( db ) > 0 ) {
    info( "Slice %#x transitioned to CONFIRMED by %s ( pid = %u ).", slice_id, hostname, getpid() );
    goto completed;
  }

  ret = execute_query( db, "delete from slices where id = %u and state = %u and "
                       "( select count(*) from ports where slice_id = %u and "
                       "( state = %u or state = %u ) ) = 0 and "
                       "( select count(*) from ports where slice_id = %u and "
                       "( state = %u or state = %u or state = %u or state = %u ) ) = 0",
                       slice_id, SLICE_STATE_DESTROYING,
                       slice_id, PORT_STATE_UPDATE_FAILED, PORT_STATE_DESTROY_FAILED,
                       slice_id, PORT_STATE_READY_TO_UPDATE, PORT_STATE_UPDATING,
                       PORT_STATE_READY_TO_DESTROY, PORT_STATE_DESTROYING );
  if ( !ret ) {
    return;
  }
  if ( mysql_affected_rows( db ) > 0 ) {
    info( "Slice %#x transitioned to DESTROYED by %s ( pid = %u ).", slice_id, hostname, getpid() );
  }

completed:
  delete_deleted_mac_addresses( slice_id );
  delete_destroyed_ports( slice_id );
}


static void
update_slice_states_on_start() {
  debug( "Updating slice states on startup." );

  update_slice_states( SLICE_STATE_READY_TO_UPDATE, SLICE_STATE_UPDATING );
  update_slice_states( SLICE_STATE_READY_TO_DESTROY, SLICE_STATE_DESTROYING );
}


static void
update_port_states_on_start( uint64_t datapath_id, uint32_t slice_id ) {
  debug( "Updating port states on startup ( slice_id = %#x, datapath_id = %#" PRIx64 " ).",  slice_id, datapath_id );

  update_port_states( datapath_id, slice_id, PORT_STATE_READY_TO_UPDATE, PORT_STATE_UPDATING );
  update_port_states( datapath_id, slice_id, PORT_STATE_READY_TO_DESTROY, PORT_STATE_DESTROYING );
}


static void
update_mac_states_on_start( uint32_t slice_id, uint32_t port_id ) {
  debug( "Updating MAC address states on startup ( slice_id = %#x, port_id = %#x ).", slice_id, port_id );

  update_mac_states( slice_id, port_id, MAC_STATE_READY_TO_INSTALL, MAC_STATE_INSTALLING );
  update_mac_states( slice_id, port_id, MAC_STATE_READY_TO_DELETE, MAC_STATE_DELETING );
}


static void
update_states_on_complete( ongoing_slice_update *update ) {
  debug( "Updating slice, port, and MAC states on complete ( update = %p ).", update );

  assert( update != NULL );
  assert( update->port_updates != NULL );

  dump_ongoing_slice_update( update );

  if ( update->n_transactions > 0 || update->n_overlay_network_transactions > 0 ) {
    return;
  }

  int n_skipped = 0;
  hash_entry *e = NULL;
  hash_iterator iter;
  init_hash_iterator( update->port_updates, &iter );
  while ( ( e = iterate_hash_next( &iter ) ) != NULL ) {
    ongoing_port_update *port_update = e->value;
    assert( port_update != NULL );
    assert( port_update->n_transactions == 0 );
    assert( port_update->n_overlay_network_transactions == 0 );
 
    dump_ongoing_port_update( port_update );

    if ( !port_update->flow_entry_installation_started ) {
      n_skipped++;
      continue;
    }

    hash_entry *e_mac = NULL;
    hash_iterator iter_mac;
    init_hash_iterator( port_update->mac_updates, &iter_mac );
    while ( ( e_mac = iterate_hash_next( &iter_mac ) ) != NULL ) {
      ongoing_mac_update *mac_update = e_mac->value;
      assert( mac_update != NULL );
      assert( mac_update->n_transactions == 0 );

      dump_ongoing_mac_update( mac_update );

      if ( !mac_update->failed ) {
        update_mac_states( update->slice_id, mac_update->port_id,
                           MAC_STATE_INSTALLING, MAC_STATE_INSTALLED );
        update_mac_states( update->slice_id, mac_update->port_id,
                           MAC_STATE_DELETING, MAC_STATE_DELETED );
      }
      else {
        update_mac_states( update->slice_id, mac_update->port_id,
                           MAC_STATE_INSTALLING, MAC_STATE_INSTALL_FAILED );
        update_mac_states( update->slice_id, mac_update->port_id,
                           MAC_STATE_DELETING, MAC_STATE_DELETE_FAILED );
      }
    }
    if ( !port_update->failed ) {
      update_port_states( port_update->datapath_id, update->slice_id,
                          PORT_STATE_UPDATING, PORT_STATE_CONFIRMED );
      update_port_states( port_update->datapath_id, update->slice_id,
                          PORT_STATE_DESTROYING, PORT_STATE_DESTROYED );
    }
    else {
      update_port_states( port_update->datapath_id, update->slice_id,
                          PORT_STATE_UPDATING, PORT_STATE_UPDATE_FAILED );
      update_port_states( port_update->datapath_id, update->slice_id,
                          PORT_STATE_DESTROYING, PORT_STATE_DESTROY_FAILED );
    }
  }

  if ( n_skipped == 0 ) {
    update_slice_state_on_complete( update->slice_id, update->failed );
    delete_ongoing_slice_update( update->slice_id );
  }
}


static bool
overlay_port_exists( uint64_t datapath_id, uint32_t slice_id ) {
  debug( "Checking if an overlay port exists or not ( slice_id = %#x, datapath_id = %#" PRIx64 " ).",
         slice_id, datapath_id );

  uint16_t port;
  char name[ IFNAMSIZ ];
  memset( name, '\0', sizeof( name ) );
  snprintf( name, sizeof( name ), "vxlan%u", slice_id );

  return get_port_no_by_name( datapath_id, name, &port );
}


static void
overlay_network_operation_completed( int status, void *user_data ) {
  debug( "An overlay network operation is completed ( status = %d, user_data = %p ).", status, user_data );

  assert( user_data != NULL );

  ongoing_port_update *update = user_data;

  dump_ongoing_port_update( update );

  assert( update->slice_update != NULL );
  if ( status == OVERLAY_NETWORK_OPERATION_FAILED ) {
    error( "Failed to execute an overlay network operation ( slice_id = %#x, datapath_id = %#" PRIx64 " ).",
           update->slice_update->slice_id, update->datapath_id );
    mark_ongoing_port_update_as_failed( update );
  }
  decrement_overlay_network_transactions( update, 1 );
  update_states_on_complete( update->slice_update );
}


static void
flow_mod_port_succeeded( uint64_t datapath_id, const buffer *message, void *user_data ) {
  debug( "A flow modification transaction is completed ( datapath_id = %#" PRIx64 ", message = %p, user_data = %p ).",
         datapath_id, message, user_data );

  assert( user_data != NULL );

  ongoing_port_update *update = user_data;

  dump_ongoing_port_update( update );

  assert( update->slice_update != NULL );
  decrement_port_transactions( update, 1 );
  update_states_on_complete( update->slice_update );
}


static void
flow_mod_port_failed( uint64_t datapath_id, const buffer *message, void *user_data ) {
  debug( "A flow modification transaction is failed ( datapath_id = %#" PRIx64 ", message = %p, user_data = %p ).",
         datapath_id, message, user_data );

  assert( user_data != NULL );

  ongoing_port_update *update = user_data;

  dump_ongoing_port_update( update );

  assert( update->slice_update != NULL );
  mark_ongoing_port_update_as_failed( update );
  decrement_port_transactions( update, 1 );
  update_states_on_complete( update->slice_update );
}


static bool
valid_vlan_id( uint16_t vid ) {
  if ( vid > 0 && vid < 0x0fff ) {
    return true;
  }

  return false;
}


static bool
add_transactions_to_install_port_flow_entries( uint64_t datapath_id, uint32_t slice_id, list_element *ports_in_slice,
                                               ongoing_port_update *update ) {
  debug( "Installing/updating flow entres ( datapath_id = %#" PRIx64 ", slice_id = %#x, ports_in_slice = %p, update = %p ).",
         datapath_id, slice_id, ports_in_slice, update );

  assert( update != NULL );
  assert( update->slice_update != NULL );

  dump_ongoing_port_update( update );

  int n_errors = 0;
  for ( list_element *e = ports_in_slice; e != NULL; e = e->next ) {
    port_in_slice *port = e->data;
    debug( "port = %u ( name = %s, vid = %#x, type = %#x ).", port->port_no, port->port_name, port->vid, port->type );

    ovs_matches *match = create_ovs_matches();
    append_ovs_match_in_port( match, port->port_no );
    if ( valid_vlan_id( port->vid ) ) {
      append_ovs_match_vlan_tci( match, ( 1 << 12 ) | port->vid, 0x1fff );
    }
    else {
      append_ovs_match_vlan_tci( match, 0, UINT16_MAX );
    }
    openflow_actions *actions = create_actions();
    append_ovs_action_reg_load( actions, 0, 32, OVSM_OVS_REG0, slice_id );
    // FIXME: hard-coded table id
    append_ovs_action_resubmit_table( actions, OFPP_IN_PORT, 2 );
    // FIXME: hard-coded table id and priority
    const uint8_t table_id = 0;
    const uint16_t priority = 128;
    buffer *flow_mod = create_ovs_flow_mod( get_transaction_id(), slice_id, OFPFC_MODIFY_STRICT,
                                            table_id, 0, 0, priority, UINT32_MAX, OFPP_NONE, 0, match, actions );
    bool ret = add_openflow_transaction( update->transactions, datapath_id, flow_mod,
                                         flow_mod_port_succeeded, update, flow_mod_port_failed, update );
    delete_ovs_matches( match );
    delete_actions( actions );
    if ( !ret ) {
      char match_str[ 256 ];
      ovs_matches_to_string( match, match_str, sizeof( match_str ) );
      error( "Failed to add an OpenFlow transaction ( datapath_id = %#" PRIx64 ", table_id = %#x, "
             "priority = %u, match = [%s] ).", datapath_id, table_id, priority, match_str );
      mark_ongoing_port_update_as_failed( update );
      n_errors++;
      break;
    }
    increment_port_transactions( update, 1 );
  }

  for ( list_element *e = ports_in_slice; e != NULL; e = e->next ) {
    port_in_slice *port = e->data;
    debug( "input port = %u ( name = %s, vid = %#x ).", port->port_no, port->port_name, port->vid );

    bool tagged = false;
    uint16_t previous_vid = OFP_VLAN_NONE;
    if ( valid_vlan_id( port->vid ) ) {
      tagged = true;
      previous_vid = port->vid;
    }

    openflow_actions *actions = create_actions();
    for ( list_element *p = ports_in_slice; p != NULL; p = p->next ) {
      port_in_slice *out_port = p->data;
      if ( out_port->port_no == port->port_no && out_port->vid == port->vid ) {
        continue;
      }
      debug( "  --> output port = %u ( name = %s, vid = %#x )", out_port->port_no, out_port->port_name, out_port->vid );

      if ( valid_vlan_id( out_port->vid ) ) {
        if ( !tagged || previous_vid != out_port->vid ) {
          append_action_set_vlan_vid( actions, out_port->vid );
          previous_vid = out_port->vid;
        }
        tagged = true;
      }
      else {
        if ( tagged ) {
          append_action_strip_vlan( actions );
          previous_vid = OFP_VLAN_NONE;
        }
        tagged = false;
      }

      if ( port->port_no != out_port->port_no ) {
        append_action_output( actions, out_port->port_no, UINT16_MAX );
      }
      else {
        append_action_output( actions, OFPP_IN_PORT, UINT16_MAX );
      }
    }

    ovs_matches *match = create_ovs_matches();
    append_ovs_match_reg( match, 0, slice_id, UINT32_MAX );
    append_ovs_match_in_port( match, port->port_no );
    if ( valid_vlan_id( port->vid ) ) {
      append_ovs_match_vlan_tci( match, ( 1 << 12 ) | port->vid, 0x1fff );
    }
    else {
      append_ovs_match_vlan_tci( match, 0, UINT16_MAX );
    }

    // FIXME: hard-coded table id and priority
    const uint8_t table_id = 2;
    const uint16_t priority = 128;
    buffer *flow_mod = create_ovs_flow_mod( get_transaction_id(), slice_id, OFPFC_MODIFY_STRICT,
                                            table_id, 0, 0, priority, UINT32_MAX, OFPP_NONE, 0, match, actions );
    bool ret = add_openflow_transaction( update->transactions, datapath_id, flow_mod,
                                         flow_mod_port_succeeded, update, flow_mod_port_failed, update );
    delete_ovs_matches( match );
    delete_actions( actions );
    if ( !ret ) {
      char match_str[ 256 ];
      ovs_matches_to_string( match, match_str, sizeof( match_str ) );
      error( "Failed to add an OpenFlow transaction ( datapath_id = %#" PRIx64 ", table_id = %#x, "
             "priority = %u, match = [%s] ).", datapath_id, table_id, priority, match_str );
      mark_ongoing_port_update_as_failed( update );
      n_errors++;
      break;
    }
    increment_port_transactions( update, 1 );
  }

  dump_ongoing_port_update( update );

  if ( n_errors > 0 ) {
    return false;
  }

  return true;
}


static bool
add_transactions_to_delete_port_flow_entries( uint64_t datapath_id, uint32_t slice_id, list_element *ports_in_slice,
                                              ongoing_port_update *update ) {
  debug( "Deleting flow entres ( datapath_id = %#" PRIx64 ", slice_id = %#x, ports_in_slice = %p, update = %p ).",
         datapath_id, slice_id, ports_in_slice, update );

  assert( update != NULL );
  assert( update->slice_update != NULL );

  dump_ongoing_port_update( update );

  int n_errors = 0;
  for ( list_element *e = ports_in_slice; e != NULL; e = e->next ) {
    port_in_slice *port = e->data;
    debug( "input port = %u ( name = %s, vid = %#x, type = %#x ).", port->port_no, port->port_name, port->vid, port->type );

    ovs_matches *match = create_ovs_matches();
    append_ovs_match_in_port( match, port->port_no );
    if ( valid_vlan_id( port->vid ) ) {
      append_ovs_match_vlan_tci( match, ( 1 << 12 ) | port->vid, 0x1fff );
    }
    else {
      append_ovs_match_vlan_tci( match, 0, UINT16_MAX );
    }

    // FIXME: hard-coded table id and priority
    const uint8_t table_id = 0;
    const uint16_t priority = 128;
    buffer *flow_mod = create_ovs_flow_mod( get_transaction_id(), slice_id, OFPFC_DELETE_STRICT,
                                            table_id, 0, 0, priority, UINT32_MAX, OFPP_NONE, 0, match, NULL );
    bool ret = add_openflow_transaction( update->transactions, datapath_id, flow_mod,
                                         flow_mod_port_succeeded, update, flow_mod_port_failed, update );
    delete_ovs_matches( match );
    if ( !ret ) {
      char match_str[ 256 ];
      ovs_matches_to_string( match, match_str, sizeof( match_str ) );
      error( "Failed to add an OpenFlow transaction ( datapath_id = %#" PRIx64 ", table_id = %#x, "
             "priority = %u, match = [%s] ).", datapath_id, table_id, priority, match_str );
      mark_ongoing_port_update_as_failed( update );
      n_errors++;
      break;
    }
    increment_port_transactions( update, 1 );
  }

  for ( list_element *e = ports_in_slice; e != NULL; e = e->next ) {
    port_in_slice *port = e->data;
    debug( "input port = %u ( name = %s, vid = %#x ).", port->port_no, port->port_name, port->vid );

    ovs_matches *match = create_ovs_matches();
    append_ovs_match_reg( match, 0, slice_id, UINT32_MAX );
    append_ovs_match_in_port( match, port->port_no );
    if ( valid_vlan_id( port->vid ) ) {
      append_ovs_match_vlan_tci( match, ( 1 << 12 ) | port->vid, 0x1fff );
    }
    else {
      append_ovs_match_vlan_tci( match, 0, UINT16_MAX );
    }

    // FIXME: hard-coded table id and priority
    const uint8_t table_id = 2;
    const uint16_t priority = 128;
    buffer *flow_mod = create_ovs_flow_mod( get_transaction_id(), slice_id, OFPFC_DELETE_STRICT,
                                            table_id, 0, 0, priority, UINT32_MAX, OFPP_NONE, 0, match, NULL );
    bool ret = add_openflow_transaction( update->transactions, datapath_id, flow_mod,
                                         flow_mod_port_succeeded, update, flow_mod_port_failed, update );
    delete_ovs_matches( match );
    if ( !ret ) {
      char match_str[ 256 ];
      ovs_matches_to_string( match, match_str, sizeof( match_str ) );
      error( "Failed to add an OpenFlow transaction ( datapath_id = %#" PRIx64 ", table_id = %#x, "
             "priority = %u, match = [%s] ).", datapath_id, table_id, priority, match_str );
      mark_ongoing_port_update_as_failed( update );
      n_errors++;
      break;
    }
    increment_port_transactions( update, 1 );
  }

  dump_ongoing_port_update( update );

  if ( n_errors > 0 ) {
    return false;
  }

  return true;
}


static void
flow_mod_mac_succeeded( uint64_t datapath_id, const buffer *message, void *user_data ) {
  debug( "A flow modification transaction is completed ( datapath_id = %#" PRIx64 ", message = %p, user_data = %p ).",
         datapath_id, message, user_data );

  assert( user_data != NULL );

  ongoing_mac_update *update = user_data;

  dump_ongoing_mac_update( update );

  assert( update->port_update != NULL );
  assert( update->port_update->slice_update != NULL );
  decrement_mac_transactions( update, 1 );
  update_states_on_complete( update->port_update->slice_update );
}


static void
flow_mod_mac_failed( uint64_t datapath_id, const buffer *message, void *user_data ) {
  debug( "A flow modification transaction is failed ( datapath_id = %#" PRIx64 ", message = %p, user_data = %p ).",
         datapath_id, message, user_data );

  assert( user_data != NULL );

  ongoing_mac_update *update = user_data;

  dump_ongoing_mac_update( update );

  assert( update->port_update != NULL );
  assert( update->port_update->slice_update != NULL );
  mark_ongoing_mac_update_as_failed( update );
  decrement_mac_transactions( update, 1 );
  update_states_on_complete( update->port_update->slice_update );
}


static bool
add_transactions_to_install_mac_flow_entries( uint64_t datapath_id, uint32_t slice_id, port_in_slice *port, list_element *mac_addresses,
                                              ongoing_mac_update *update ) {
  debug( "Installing/updating flow entres ( datapath_id = %#" PRIx64 ", slice_id = %#x, port = %p, mac_addresses = %p, update = %p ).",
         datapath_id, slice_id, port, mac_addresses, update );

  assert( port != NULL );

  dump_port_in_slice( port );

  assert( update != NULL );
  assert( update->port_update != NULL );

  dump_ongoing_mac_update( update );

  int n_errors = 0;
  for ( list_element *e = mac_addresses; e != NULL; e = e->next ) {
    uint64_t *address = e->data;

    debug( "mac address = %#" PRIx64, *address & MAC_ADDRESS_MASK );

    ovs_matches *match = NULL;
    buffer *flow_mod = NULL;
    uint8_t table_id = 0;
    uint16_t priority = 0;

    uint8_t addr[ OFP_ETH_ALEN] = { 0, 0, 0, 0, 0, 0 };
    uint8_t mask[ OFP_ETH_ALEN ] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
    addr[ 0 ] = ( *address >> 40 ) & 0xff;
    addr[ 1 ] = ( *address >> 32 ) & 0xff;
    addr[ 2 ] = ( *address >> 24 ) & 0xff;
    addr[ 3 ] = ( *address >> 16 ) & 0xff;
    addr[ 4 ] = ( *address >> 8 ) & 0xff;
    addr[ 5 ] = *address & 0xff;

    // FIXME: hard-coded table id and priority
    if ( port->type == PORT_TYPE_CUSTOMER ) {
      match = create_ovs_matches();
      append_ovs_match_in_port( match, port->port_no );
      if ( valid_vlan_id( port->vid ) ) {
        append_ovs_match_vlan_tci( match, ( 1 << 12 ) | port->vid, 0x1fff );
      }
      else {
        append_ovs_match_vlan_tci( match, 0, UINT16_MAX );
      }
      append_ovs_match_eth_dst( match, addr, mask );

      // FIXME: hard-coded table id and priority
      table_id = 0;
      priority = 512;
      flow_mod = create_ovs_flow_mod( get_transaction_id(), slice_id, OFPFC_MODIFY_STRICT,
                                      table_id, 0, 0, priority, UINT32_MAX, OFPP_NONE, 0, match, NULL );
      bool ret = add_openflow_transaction( update->transactions, datapath_id, flow_mod,
                                           flow_mod_mac_succeeded, update, flow_mod_mac_failed, update );
      delete_ovs_matches( match );
      if ( !ret ) {
        char match_str[ 256 ];
        ovs_matches_to_string( match, match_str, sizeof( match_str ) );
        error( "Failed to add an OpenFlow transaction ( datapath_id = %#" PRIx64 ", table_id = %#x, "
               "priority = %u, match = [%s] ).", datapath_id, table_id, priority, match_str );
        mark_ongoing_mac_update_as_failed( update );
        n_errors++;
      }
      else {
        increment_mac_transactions( update, 1 );
      }
    }

    match = create_ovs_matches();
    append_ovs_match_reg( match, 0, slice_id, UINT32_MAX );
    append_ovs_match_eth_dst( match, addr, mask );
    openflow_actions *actions = create_actions();
    if ( valid_vlan_id( port->vid ) ) {
      append_action_set_vlan_vid( actions, port->vid );
    }
    else {
      append_action_strip_vlan( actions );
    }
    append_action_output( actions, port->port_no, UINT16_MAX );
    table_id = 2;
    if ( port->type == PORT_TYPE_CUSTOMER ) {
      priority = 512;
    }
    else if ( port->type == PORT_TYPE_OVERLAY ) {
      priority = 256;
    }
    else {
      error( "Invalid port type ( datapath_id = %#" PRIx64 ", port_id = %#x, port_no = %u, vid = %#x, port_name = %s, port_type = %#x ).",
             port->datapath_id, port->port_id, port->port_no, port->vid, port->port_name, port->type );
      mark_ongoing_mac_update_as_failed( update );
      n_errors++;
      break;
    }

    flow_mod = create_ovs_flow_mod( get_transaction_id(), slice_id, OFPFC_MODIFY_STRICT,
                                    table_id, 0, 0, priority, UINT32_MAX, OFPP_NONE, 0, match, actions );
    bool ret = add_openflow_transaction( update->transactions, datapath_id, flow_mod,
                                         flow_mod_mac_succeeded, update, flow_mod_mac_failed, update );
    delete_ovs_matches( match );
    delete_actions( actions );
    if ( !ret ) {
      char match_str[ 256 ];
      ovs_matches_to_string( match, match_str, sizeof( match_str ) );
      error( "Failed to add an OpenFlow transaction ( datapath_id = %#" PRIx64 ", table_id = %#x, "
             "priority = %u, match = [%s] ).", datapath_id, table_id, priority, match_str );
      mark_ongoing_mac_update_as_failed( update );
      n_errors++;
    }
    else {
      increment_mac_transactions( update, 1 );
    }
  }

  dump_ongoing_mac_update( update );

  if ( n_errors > 0 ) {
    return false;
  }

  return true;
}


static bool
add_transactions_to_delete_mac_flow_entries( uint64_t datapath_id, uint32_t slice_id, port_in_slice *port, list_element *mac_addresses,
                                            ongoing_mac_update *update ) {
  debug( "Deleting flow entres ( datapath_id = %#" PRIx64 ", slice_id = %#x, port = %p, mac_addresses = %p, update = %p ).",
         datapath_id, slice_id, port, mac_addresses, update );

  assert( port != NULL );

  dump_port_in_slice( port );

  assert( update != NULL );
  assert( update->port_update != NULL );

  dump_ongoing_mac_update( update );

  int n_errors = 0;
  for ( list_element *e = mac_addresses; e != NULL; e = e->next ) {
    uint64_t *address = e->data;

    debug( "mac address = %#" PRIx64, *address & MAC_ADDRESS_MASK );

    ovs_matches *match = NULL;
    uint8_t table_id = 0;
    uint16_t priority = 0;
    buffer *flow_mod = NULL;

    uint8_t addr[ OFP_ETH_ALEN] = { 0, 0, 0, 0, 0, 0 };
    uint8_t mask[ OFP_ETH_ALEN ] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
    addr[ 0 ] = ( *address >> 40 ) & 0xff;
    addr[ 1 ] = ( *address >> 32 ) & 0xff;
    addr[ 2 ] = ( *address >> 24 ) & 0xff;
    addr[ 3 ] = ( *address >> 16 ) & 0xff;
    addr[ 4 ] = ( *address >> 8 ) & 0xff;
    addr[ 5 ] = *address & 0xff;

    if ( port->type == PORT_TYPE_CUSTOMER ) {
      match = create_ovs_matches();
      append_ovs_match_in_port( match, port->port_no );
      if ( valid_vlan_id( port->vid ) ) {
        append_ovs_match_vlan_tci( match, ( 1 << 12 ) | port->vid, 0x1fff );
      }
      else {
        append_ovs_match_vlan_tci( match, 0, UINT16_MAX );
      }
      append_ovs_match_eth_dst( match, addr, mask );

      // FIXME: hard-coded table id and priority
      table_id = 0;
      priority = 512;
      flow_mod = create_ovs_flow_mod( get_transaction_id(), slice_id, OFPFC_DELETE_STRICT,
                                      table_id, 0, 0, priority, UINT32_MAX, OFPP_NONE, 0, match, NULL );
      bool ret = add_openflow_transaction( update->transactions, datapath_id, flow_mod,
                                           flow_mod_mac_succeeded, update, flow_mod_mac_failed, update );
      delete_ovs_matches( match );
      if ( !ret ) {
        char match_str[ 256 ];
        ovs_matches_to_string( match, match_str, sizeof( match_str ) );
        error( "Failed to add an OpenFlow transaction ( datapath_id = %#" PRIx64 ", table_id = %#x, "
               "priority = %u, match = [%s] ).", datapath_id, table_id, priority, match_str );
        mark_ongoing_mac_update_as_failed( update );
        n_errors++;
      }
      else {
        increment_mac_transactions( update, 1 );
      }
    }

    match = create_ovs_matches();
    append_ovs_match_reg( match, 0, slice_id, UINT32_MAX );
    append_ovs_match_eth_dst( match, addr, mask );
    table_id = 2;
    if ( port->type == PORT_TYPE_CUSTOMER ) {
      priority = 512;
    }
    else if ( port->type == PORT_TYPE_OVERLAY ) {
      priority = 256;
    }
    else {
      error( "Invalid port type ( datapath_id = %#" PRIx64 ", port_id = %#x, port_no = %u, vid = %#x, port_name = %s, port_type = %#x ).",
             port->datapath_id, port->port_id, port->port_no, port->vid, port->port_name, port->type );
      mark_ongoing_mac_update_as_failed( update );
      n_errors++;
      break;
    }

    flow_mod = create_ovs_flow_mod( get_transaction_id(), slice_id, OFPFC_DELETE_STRICT,
                                    table_id, 0, 0, priority, UINT32_MAX, port->port_no, 0, match, NULL );
    bool ret = add_openflow_transaction( update->transactions, datapath_id, flow_mod,
                                         flow_mod_mac_succeeded, update, flow_mod_mac_failed, update );
    delete_ovs_matches( match );
    if ( !ret ) {
      char match_str[ 256 ];
      ovs_matches_to_string( match, match_str, sizeof( match_str ) );
      error( "Failed to add an OpenFlow transaction ( datapath_id = %#" PRIx64 ", table_id = %#x, "
             "priority = %u, match = [%s] ).", datapath_id, table_id, priority, match_str );
      mark_ongoing_mac_update_as_failed( update );
      n_errors++;
    }
    else {
      increment_mac_transactions( update, 1 );
    }
  }

  dump_ongoing_mac_update( update );

  if ( n_errors > 0 ) {
    return false;
  }

  return true;
}


static void
check_mac_entries_of_active_port( ongoing_port_update *port_update, port_in_slice *port ) {
  debug( "Checking MAC addresses associated with a port ( port_update = %p, port_in_slice = %p ).", port_update, port );

  assert( port_update != NULL );
  assert( port != NULL );

  dump_ongoing_port_update( port_update );

  uint32_t slice_id = port_update->slice_update->slice_id;

  update_mac_states_on_start( slice_id, port->port_id );
  ongoing_mac_update *mac_update = add_ongoing_mac_update( port_update, port->port_id );

  int n_addresses = 0;
  list_element *mac_addresses_to_install = NULL;
  bool ret = get_mac_addresses_to_install( slice_id, port->port_id, &mac_addresses_to_install, &n_addresses );
  if ( ret ) {
    info( "%d MAC addresses need to be installed to switch %#" PRIx64 " ( slice_id = %#x, port_id = %#x ).",
          n_addresses, port->datapath_id, slice_id, port->port_id );
    if ( n_addresses > 0 && mac_addresses_to_install != NULL ) {
      add_transactions_to_install_mac_flow_entries( port_update->datapath_id, slice_id, port,
                                                    mac_addresses_to_install, mac_update );
    }
  }
  else {
    mark_ongoing_mac_update_as_failed( mac_update );
  }
  free_list( mac_addresses_to_install );

  list_element *mac_addresses_to_delete = NULL;
  ret = get_mac_addresses_to_delete( slice_id, port->port_id, &mac_addresses_to_delete, &n_addresses );
  if ( ret ) {
    info( "%d MAC addresses need to be deleted from switch %#" PRIx64 " ( slice_id = %#x, port_id = %#x ).",
          n_addresses, port->datapath_id, slice_id, port->port_id );
    if ( n_addresses > 0 && mac_addresses_to_delete != NULL ) {
      add_transactions_to_delete_mac_flow_entries( port_update->datapath_id, slice_id, port,
                                                   mac_addresses_to_delete, mac_update );
    }
  }
  else {
    mark_ongoing_mac_update_as_failed( mac_update );
  }
  free_list( mac_addresses_to_delete );

  dump_ongoing_port_update( port_update );
}


static void
check_mac_entries_of_inactive_port( ongoing_port_update *port_update, port_in_slice *port ) {
  debug( "Checking MAC addresses detached from a port ( port_update = %p, port_in_slice = %p ).", port_update, port );

  assert( port_update != NULL );
  assert( port != NULL );

  dump_ongoing_port_update( port_update );

  uint32_t slice_id = port_update->slice_update->slice_id;

  update_mac_states_on_start( slice_id, port->port_id );
  ongoing_mac_update *mac_update = add_ongoing_mac_update( port_update, port->port_id );

  int n_addresses = 0;
  list_element *mac_addresses_to_delete = NULL;
  bool ret = get_mac_addresses_to_delete( slice_id, port->port_id, &mac_addresses_to_delete, &n_addresses );
  if ( ret ) {
    info( "%d MAC addresses need to be deleted from switch %#" PRIx64 " ( slice_id = %#x, port_id = %#x ).",
          n_addresses, port->datapath_id, slice_id, port->port_id );
    if ( n_addresses > 0 && mac_addresses_to_delete != NULL ) {
      add_transactions_to_delete_mac_flow_entries( port_update->datapath_id, slice_id, port,
                                                   mac_addresses_to_delete, mac_update );
    }
  }
  else {
    mark_ongoing_mac_update_as_failed( mac_update );
  }
  free_list( mac_addresses_to_delete );

  dump_ongoing_port_update( port_update );
}


static bool
attach_to_overlay_network( ongoing_port_update *port_update, port_in_slice *port ) {
  debug( "Attaching a switch port to a slice( port_update = %p, port = %p ).", port_update, port );

  assert( port_update != NULL );
  assert( port_update->slice_update != NULL );
  assert( port != NULL );

  dump_ongoing_port_update( port_update );

  uint32_t slice_id = port_update->slice_update->slice_id;

  info( "Attaching a switch port ( datapath_id = %#" PRIx64 ", name = %s ) to a slice ( %#x ).",
        port->datapath_id, port->port_name, slice_id );

  if ( port->type != PORT_TYPE_OVERLAY ) {
    warn( "Invalid port type ( %#x ).", port->type );
    return false;
  }
  if ( overlay_port_exists( port_update->datapath_id, slice_id ) ) {
    debug( "An overlay port already exists ( port_name = %s, slice_id = %#x ).", port->port_name, slice_id );
    return true;
  }

  int ret = attach_to_network( port->datapath_id, slice_id, overlay_network_operation_completed, port_update );
  if ( ret != OVERLAY_NETWORK_OPERATION_SUCCEEDED ) {
    if ( ret == OVERLAY_NETWORK_OPERATION_IN_PROGRESS ) {
      return true;
    }
    else if ( ret == OVERLAY_NETWORK_OPERATION_FAILED ) {
      error( "Failed to execute an overlay network operation ( ret = %d ).", ret );
      return false;
    }
  }

  increment_overlay_network_transactions( port_update, 1 );

  dump_ongoing_port_update( port_update );

  return true;
}


static bool
detach_from_overlay_network( ongoing_port_update *port_update, port_in_slice *port ) {
  debug( "Detaching a switch port from a slice ( port_update = %p, port = %p ).", port_update, port );

  assert( port_update != NULL );
  assert( port_update->slice_update != NULL );
  assert( port != NULL );

  dump_ongoing_port_update( port_update );

  uint32_t slice_id = port_update->slice_update->slice_id;

  info( "Detaching a switch port ( datapath_id = %#" PRIx64 ", name = %s ) from a slice ( %#x ).",
        port->datapath_id, port->port_name, slice_id );

  if ( port->type != PORT_TYPE_OVERLAY ) {
    warn( "Invalid port type ( %#x ).", port->type );
    return false;
  }
  if ( !overlay_port_exists( port_update->datapath_id, slice_id ) ) {
    warn( "An overlay port does not exist ( port_name = %s, slice_id = %#x ).", port->port_name, slice_id );
    return true;
  }

  int ret = detach_from_network( port->datapath_id, slice_id, overlay_network_operation_completed, port_update );
  if ( ret != OVERLAY_NETWORK_OPERATION_SUCCEEDED ) {
    if ( ret == OVERLAY_NETWORK_OPERATION_IN_PROGRESS ) {
      return true;
    }
    else if ( ret == OVERLAY_NETWORK_OPERATION_FAILED ) {
      error( "Failed to execute an overlay network operation ( ret = %d ).", ret );
      return false;
    }
  }

  increment_overlay_network_transactions( port_update, 1 );

  info( "Detaching a switch port ( %s ) from a slice ( %#x ).", port->port_name, slice_id );

  return true;
}


static void
check_ports_to_update( ongoing_slice_update *slice_update, uint64_t datapath_id ) {
  debug( "Checking switch ports to be updated on a switch ( slice_update = %p, datapath_id = %#" PRIx64 " ).", slice_update, datapath_id );

  assert( slice_update != NULL );

  dump_ongoing_slice_update( slice_update );

  ongoing_port_update *port_update = add_ongoing_port_update( slice_update, datapath_id );

  dump_ongoing_port_update( port_update );

  if ( port_update->flow_entry_installation_started ) {
    flush_openflow_transactions( port_update );
    return;
  }

  update_port_states_on_start( datapath_id, slice_update->slice_id );

  if ( port_update->slice_update->failed ) {
    port_update->flow_entry_installation_started = true;
    return;
  }

  list_element *overlay_port_to_attach = NULL;
  int n_ports = 0;
  bool ret = get_activating_overlay_port( slice_update->slice_id, datapath_id, &overlay_port_to_attach, &n_ports );
  if ( ret == false ) {
    error( "Failed to retrieve an overlay port to attach ( datapath_id = %#" PRIx64 ", slice_id = %#x ).",
           datapath_id, slice_update->slice_id );
    mark_ongoing_port_update_as_failed( port_update );
    return;
  }
  assert( n_ports <= 1 );
  for ( list_element *e = overlay_port_to_attach; e != NULL; e = e->next ) {
    port_in_slice *port = e->data;
    ret = attach_to_overlay_network( port_update, port );
    if ( ret == false ) {
      mark_ongoing_port_update_as_failed( port_update );
      free_list( overlay_port_to_attach );
      return;
    }
  }

  free_list( overlay_port_to_attach );

  assert( port_update->slice_update != NULL );
  if ( port_update->slice_update->n_overlay_network_transactions > 0 ) {
    return;
  }

  list_element *active_ports_in_slice = NULL;
  n_ports = 0;
  ret = get_active_ports_in_slice( datapath_id, slice_update->slice_id, &active_ports_in_slice, &n_ports );
  if ( !ret ) {
    error( "Failed to retrieve active/activating ports ( datapath_id = %#" PRIx64 ", slice_id = %#x ).",
           datapath_id, slice_update->slice_id );
    mark_ongoing_port_update_as_failed( port_update );
    return;
  }

  info( "%d active/activating ports in slice ( datapath_id = %#" PRIx64 ", slice_id = %#x ).",
        n_ports, datapath_id, slice_update->slice_id );

  list_element *inactive_ports_in_slice = NULL;
  ret = get_inactive_ports_in_slice( datapath_id, slice_update->slice_id, &inactive_ports_in_slice, &n_ports );
  if ( !ret ) {
    error( "Failed to retrieve inactive/inactivating ports ( datapath_id = %#" PRIx64 ", slice_id = %#x ).",
           datapath_id, slice_update->slice_id );
    free_list( active_ports_in_slice );
    mark_ongoing_port_update_as_failed( port_update );
    return;
  }

  info( "%d inactive/inactivating ports in slice ( datapath_id = %#" PRIx64 ", slice_id = %#x ).",
        n_ports, datapath_id, slice_update->slice_id );

  if ( active_ports_in_slice != NULL ) {
    add_transactions_to_install_port_flow_entries( datapath_id, slice_update->slice_id, active_ports_in_slice, port_update );

    for ( list_element *p = active_ports_in_slice; p != NULL; p = p->next ) {
      port_in_slice *port = p->data;
      check_mac_entries_of_active_port( port_update, port );
    }

    free_list( active_ports_in_slice );
  }

  if ( inactive_ports_in_slice != NULL ) {
    for ( list_element *p = inactive_ports_in_slice; p != NULL; p = p->next ) {
      port_in_slice *port = p->data;
      check_mac_entries_of_inactive_port( port_update, port );
    }

    add_transactions_to_delete_port_flow_entries( datapath_id, slice_update->slice_id, inactive_ports_in_slice, port_update );

    free_list( inactive_ports_in_slice );
  }

  flush_openflow_transactions( port_update );

  list_element *overlay_port_to_detach = NULL;
  n_ports = 0;
  ret = get_inactivating_overlay_port( slice_update->slice_id, datapath_id, &overlay_port_to_detach, &n_ports );
  if ( ret == false ) {
    error( "Failed to retrieve an overlay port to detach ( datapath_id = %#" PRIx64 ", slice_id = %#x ).",
           datapath_id, slice_update->slice_id );
    mark_ongoing_port_update_as_failed( port_update );
    return;
  }
  assert( n_ports <= 1 );
  for ( list_element *e = overlay_port_to_detach; e != NULL; e = e->next ) {
    port_in_slice *port = e->data;
    ret = detach_from_overlay_network( port_update, port );
    if ( ret == false ) {
      mark_ongoing_port_update_as_failed( port_update );
      free_list( overlay_port_to_detach );
      return;
    }
  }

  free_list( overlay_port_to_detach );

  port_update->flow_entry_installation_started = true;

  dump_ongoing_slice_update( slice_update );
}


static void
check_switches_to_update( uint32_t slice_id ) {
  info( "Checking a slice ( slice_id = %#x ).", slice_id );

  ongoing_slice_update *slice_update = add_ongoing_slice_update( slice_id );

  list_element *switches_to_update = NULL;
  int n_switches = 0;
  bool ret = get_switches_to_update( slice_id, &switches_to_update, &n_switches );
  if ( ret == false ) {
    error( "Failed to retrieve the list of switches to be updated ( slice_id = %#x ).", slice_id );
    goto check_completed;
  }

  for ( list_element *sw = switches_to_update; sw != NULL; sw = sw->next ) {
    uint64_t *datapath_id = sw->data;
    info( "Updating switch ( slice_id = %#x, datapath_id = %#" PRIx64 " ).", slice_id, *datapath_id );
    check_ports_to_update( slice_update, *datapath_id );
  }
  free_list( switches_to_update );

check_completed:
  update_states_on_complete( slice_update );
}


static void
check_slice_db( void *user_data ) {
  UNUSED( user_data );

  debug( "Loading slice definitions." );

  update_slice_states_on_start();

  list_element *slices_to_update = NULL;
  int n_slices = 0;
  bool ret = get_slices_to_update( &slices_to_update, &n_slices );
  if ( ret == false ) { 
    error( "Failed to retrieve the list of slices to be updated." );
    return;
  }
  if ( n_slices == 0 ) {
    debug( "No slices to be updated." );
    return;
  }

  for ( list_element *e = slices_to_update; e != NULL; e = e->next ) {
    uint32_t *slice_id = e->data;
    check_switches_to_update( *slice_id );
  }

  free_list( slices_to_update );
}


static void
handle_switch_port_event( uint8_t event, uint64_t datapath_id, const char *name, uint16_t number, void *user_data ) {
  debug( "Handling a switch port event ( event = %#x, datapath_id = %#" PRIx64 ", name = %s, number = %u, user_data = %p ).",
         event, datapath_id, name != NULL ? name : "", number, user_data );

  assert( db != NULL );

  bool ret = false;
  if ( event == SWITCH_PORT_ADDED ) {
    assert( name != NULL );
    if ( strlen( name ) > 0 ) {
      ret = execute_query( db, "update ports set port_no = %u where datapath_id = %" PRIu64 " and port_name = \'%s\'",
                           number, datapath_id, name );
    }
  }
  else if ( event == SWITCH_PORT_DELETED ) {
    if ( number != OFPP_ALL ) {
      assert( name != NULL );
      if ( strlen( name ) > 0 ) {
        ret = execute_query( db, "update ports set port_no = %u where datapath_id = %" PRIu64 " and port_name = \'%s\'",
                             OFPP_NONE, datapath_id, name );
      }
    }
    else {
      ret = execute_query( db, "update ports set port_no = %u where datapath_id = %" PRIu64 " and length(port_name) > 0",
                           OFPP_NONE, datapath_id );
    }
  }
  else {
    error( "Undefined switch port event ( %#x ).", event );
    return;
  }

  if ( ret ) {
    debug( "Port number is updated ( event = %#x, datapath_id = %#" PRIx64 ", name = %s, number = %u, user_data = %p ).",
           event, datapath_id, name != NULL ? name : "", number, user_data );
  }
}


bool
init_slice( MYSQL *_db ) {
  debug( "Initializing slice manager ( _db = %p ).", _db );

  assert( _db != NULL );
  assert( db == NULL );
  assert( slice_updates == NULL );

  memset( hostname, '\0', sizeof( hostname ) );
  gethostname( hostname, HOST_NAME_MAX );
  db = _db;

  slice_updates = create_hash_with_size( compare_ongoing_slice_update, hash_ongoing_slice_update, 1024 );
  assert( slice_updates != NULL );

  set_switch_port_event_handler( handle_switch_port_event, NULL );

  struct itimerspec interval;
  interval.it_interval = SLICE_DB_CHECK_INTERVAL;
  interval.it_value = SLICE_DB_CHECK_INTERVAL;
  add_timer_event_callback( &interval, check_slice_db, NULL );

  debug( "Initialization completed." );

  return true;
}


bool
finalize_slice() {
  debug( "Finalizing slice manager." );

  assert( db != NULL );
  assert( slice_updates != NULL );

  delete_timer_event( check_slice_db, NULL );
  memset( hostname, '\0', sizeof( hostname ) );
  db = NULL;

  hash_entry *e = NULL;
  hash_iterator iter;
  init_hash_iterator( slice_updates, &iter );
  while ( ( e = iterate_hash_next( &iter ) ) != NULL ) {
    ongoing_slice_update *update = e->value;
    if ( update != NULL ) {
      free_ongoing_slice_update( update );
    }
  }
  delete_hash( slice_updates );

  set_switch_port_event_handler( NULL, NULL );

  debug( "Finalization completed." );

  return true;
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
