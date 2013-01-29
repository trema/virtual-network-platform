/*
 * Author: Yasunobu Chiba and Satoshi Tsuyama
 *
 * Copyright (C) 2012 NEC Corporation
 * Copyright (C) NEC BIGLOBE, Ltd. 2012
 * NEC Confidential
 */


#include <arpa/inet.h>
#include <assert.h>
#include <json/json.h>
#include "db.h"
#include "http_client.h"
#include "switch.h"
#include "overlay_network_manager.h"
#include "utilities.h"
#ifdef UNIT_TEST
#include "overlay_network_manager_unittest.h"
#endif


enum {
  OPERATION_ATTACH,
  OPERATION_DETACH,
};


typedef struct {
  uint64_t datapath_id;
  uint32_t vni;
  int operation;
  int port_wait_count;
  unsigned int n_ongoing_http_requests;
  unsigned int n_failed_http_requests;
  overlay_operation_completed_handler callback;
  void *user_data;
} transaction_entry;


static MYSQL *db = NULL;
static const uint32_t VNI_MASK = 0x00ffffff;
static const size_t MAX_VNI_STRLEN = 9;
static hash_table *transactions = NULL;


static bool
valid_vni( uint32_t vni ) {
  debug( "Validating a VNI ( %#x ).", vni );

  if ( ( vni & ~VNI_MASK ) != 0 ) {
    error( "Invalid VNI value ( %#x ).", vni );
    return false;
  }

  debug( "Valid VNI ( %#x ).", vni );

  return true;
}


static bool
valid_reflector_address( const char *address ) {
  debug( "Validating an IPv4 address ( %s ).", address != NULL ? address : "" );

  assert( address != NULL );

  struct in_addr dst;
  int ret = inet_pton( AF_INET, address, &dst );
  if ( ret != 1 ) {
    error( "Invalid IP address ( %s ).", address );
    return false;
  }

  const uint32_t dst_addr = ntohl( dst.s_addr );
  if ( ( IN_CLASSA( dst_addr ) && ( dst_addr != INADDR_ANY ) && ( dst_addr != INADDR_LOOPBACK ) ) ||
       IN_CLASSB( dst_addr ) || IN_CLASSC( dst_addr ) || IN_CLASSD( dst_addr ) ) {
    debug( "Valid IPv4 address ( %s ).", address );
    return true;
  }

  error( "Invalid IP address ( %s ).", address );

  return false;
}


static bool
valid_multicast_address( const char *address ) {
  assert( address != NULL );

  struct in_addr dst;
  int ret = inet_pton( AF_INET, address, &dst );
  if ( ret != 1 ) {
    error( "Invalid multicast address ( %s ).", address );
    return false;
  }

  const uint32_t dst_addr = ntohl( dst.s_addr );
  if ( !IN_MULTICAST( dst_addr ) ) {
    return false;
  }

  return true;
}


static bool
create_json_to_add_tep( char **json_string, const char *address, uint16_t port ) {
  debug( "Creating a json string ( json_string = %p, address = %s, port = %u ).", json_string, address != NULL ? address : "", port );

  assert( json_string != NULL );
  assert( address != NULL );
  assert( port > 0 );

  json_object *root_object = NULL;
  json_object *ip_object = NULL;
  json_object *port_object = NULL;

  *json_string = NULL;

  if ( !valid_reflector_address( address ) ) {
    return false;
  }

  root_object = json_object_new_object();
  if ( root_object == NULL ) {
    error( "Failed to create a json object." );
    goto error;
  }

  ip_object = json_object_new_string( address );
  if ( ip_object == NULL ) {
    error( "Failed to create a json object for ip." );
    goto error;
  }
  json_object_object_add( root_object, "ip", ip_object );

  port_object = json_object_new_int( ( int32_t ) port );
  if ( port_object == NULL ) {
    error( "Failed to create a json object for port." );
    goto error;
  }
  json_object_object_add( root_object, "port", port_object );

  const char *string = json_object_to_json_string( root_object );
  if ( string == NULL ) {
    error( "Failed to translate a json object to string ( root_object = %p ).", root_object );
    goto error;
  }

  *json_string = xstrdup( string );

  debug( "Json string is created successfully ( root_object = %p, string = %s ).", root_object, *json_string );

  json_object_put( root_object );

  return true;

error:
  if ( root_object != NULL ) {
    json_object_put( root_object );
  }
  if ( *json_string != NULL ) {
    xfree( *json_string );
    *json_string = NULL;
  }

  return false;
}


static bool
create_json_to_add_tunnel( char **json_string, uint32_t vni, const char *address, uint16_t port ) {
  debug( "Creating a json string ( json_string = %p, vni = %#x, address = %s, port = %u ).",
         json_string, vni, address !=NULL ? address : "", port );

  assert( json_string != NULL );
  assert( address != NULL );
  assert( port > 0 );

  json_object *root_object = NULL;
  json_object *vni_object = NULL;
  json_object *broadcast_object = NULL;

  *json_string = NULL;

  if ( !valid_vni( vni ) ) {
    return false;
  }
  if ( !valid_reflector_address( address ) ) {
    return false;
  }

  root_object = json_object_new_object();
  if ( root_object == NULL ) {
    error( "Failed to create a json object." );
    goto error;
  }

  vni_object = json_object_new_int( ( int32_t ) vni );
  if ( vni_object == NULL ) {
    error( "Failed to create a json object for vni." );
    goto error;
  }
  json_object_object_add( root_object, "vni", vni_object );

  broadcast_object = json_object_new_string( address );
  if ( broadcast_object == NULL ) {
    error( "Failed to create a json object for broadcast address." );
    goto error;
  }
  json_object_object_add( root_object, "broadcast", broadcast_object );

  const char *string = json_object_to_json_string( root_object );
  if ( string == NULL ) {
    error( "Failed to translate a json object to string ( root_object = %p ).", root_object );
    goto error;
  }

  *json_string = xstrdup( string );

  debug( "Json string is created successfully ( root_object = %p, string = %s ).", root_object, *json_string );

  json_object_put( root_object );

  return true;

error:
  if ( root_object != NULL ) {
    json_object_put( root_object );
  }
  if ( *json_string != NULL ) {
    xfree( *json_string );
    *json_string = NULL;
  }

  return false;
}


static unsigned int
hash_transaction( const void *key ) {
  const transaction_entry *transaction = key;

  return ( unsigned int ) transaction->datapath_id;
}


static bool
compare_transaction( const void *x, const void *y ) {
  const transaction_entry *transaction_x = x;
  const transaction_entry *transaction_y = y;

  if ( transaction_x->datapath_id == transaction_y->datapath_id &&
       transaction_x->vni == transaction_y->vni ) {
    return true;
  }

  return false;
}


static bool
create_transaction_db() {
  debug( "Creating transaction database for overlay network manager." );

  assert( transactions == NULL );

  transactions = create_hash( compare_transaction, hash_transaction );
  assert( transactions != NULL );

  debug( "Transaction database is created ( %p ).", transactions );

  return true;
}


static bool
delete_transaction_db() {
  debug( "Deleting transaction database for overlay network manager ( %p ).", transactions );

  assert( transactions != NULL );

  hash_entry *e = NULL;
  hash_iterator iter;
  init_hash_iterator( transactions, &iter );
  while ( ( e = iterate_hash_next( &iter ) ) != NULL ) {
    if ( e->value != NULL ) {
      xfree( e->value );
      e->value = NULL;
    }
  }

  delete_hash( transactions );

  debug( "Transaction database is deleted ( %p ).", transactions );

  transactions = NULL;

  return true;
}


static bool
add_transaction( uint64_t datapath_id, uint32_t vni, int operation,
                 overlay_operation_completed_handler callback, void *user_data ) {
  debug( "Adding a transaction record ( datapath_id = %#" PRIx64 ", vni = %#x, operation = %#x, callback = %p, user_data = %p ).",
         datapath_id, vni, operation, callback, user_data );

  if ( !valid_vni( vni ) ) {
    return false;
  }

  transaction_entry *entry = xmalloc( sizeof( transaction_entry ) );
  memset( entry, 0, sizeof( transaction_entry ) );
  entry->datapath_id = datapath_id;
  entry->vni = vni;
  entry->operation = operation;
  entry->port_wait_count = 0;
  entry->n_ongoing_http_requests = 0;
  entry->n_failed_http_requests = 0;
  entry->callback = callback;
  entry->user_data = user_data;

  entry = insert_hash_entry( transactions, entry, entry );
  if ( entry != NULL ) {
    warn( "Duplicated transaction record found ( datapath_id = %#" PRIx64 ", vni = %#x, operation = %#x, callback = %p, user_data = %p ).",
           datapath_id, vni, operation, callback, user_data );
    xfree( entry );
  }
  debug( "A transaction record is added ( datapath_id = %#" PRIx64 ", vni = %#x, operation = %#x, callback = %p, user_data = %p ).",
         datapath_id, vni, operation, callback, user_data );

  return true;
}


static bool
delete_transaction( uint64_t datapath_id, uint32_t vni ) {
  debug( "Deleting a transaction record ( datapath_id = %#" PRIx64 ", vni = %#x ).", datapath_id, vni );

  if ( !valid_vni( vni ) ) {
    return false;
  }

  transaction_entry key = { datapath_id, vni, 0, 0, 0, 0, NULL, NULL };
  transaction_entry *entry = delete_hash_entry( transactions, &key );
  if ( entry == NULL ) {
    error( "Failed to find a transaction entry ( datapath_id %" PRIx64 ", vni = %#x ).",
           key.datapath_id, key.vni );
    return false;
  }
  xfree( entry );

  debug( "A transaction record is deleted ( datapath_id = %#" PRIx64 ", vni = %#x ).", datapath_id, vni );

  return true;
}


static transaction_entry *
lookup_transaction( uint64_t datapath_id, uint32_t vni ) {
  debug( "Looking up a transaction record ( datapath_id = %#" PRIx64 ", vni = %#x ).", datapath_id, vni );

  if ( !valid_vni( vni ) ) {
    return NULL;
  }

  transaction_entry key = { datapath_id, vni, 0, 0, 0, 0, NULL, NULL };
  transaction_entry *entry = lookup_hash_entry( transactions, &key );
  if ( entry != NULL ) {
    debug( "Transaction record found ( datapath_id = %#" PRIx64 ", vni = %#x, operation = %#x, port_wait_count = %d, callback = %p, user_data = %p ).",
           entry->datapath_id, entry->vni, entry->operation, entry->port_wait_count, entry->callback, entry->user_data );
  }
  else {
    debug( "Failed to lookup a transaction record ( datapath_id = %#" PRIx64 ", vni = %#x ).", datapath_id, vni );
  }

  return entry;
}


static bool
string_to_uint16( const char *string, uint16_t *value ) {
  assert( string != NULL );
  assert( value != NULL );

  char *endp = NULL;
  unsigned long int ul = strtoul( string, &endp, 0 );
  if ( *endp != '\0' ) {
    return false;
  }

  if ( ul > UINT16_MAX ) {
    return false;
  }

  *value = ( uint16_t ) ul;

  return true;
}


static bool
get_overlay_info( uint32_t vni, uint64_t datapath_id, char **local_address, uint16_t *local_port,
                  char **broadcast_address, uint16_t *broadcast_port ) {
  debug( "Retrieving overlay network information from database ( vni = %#x, datapath_id = %#" PRIx64 ", local_address = %p, local_port = %p, "
         "broadcast_address = %p, broadcast_port = %p ).", vni, datapath_id, local_address, local_port, broadcast_address, broadcast_port );

  assert( db != NULL );
  assert( local_address != NULL );
  assert( broadcast_address != NULL );

  MYSQL_RES *result = NULL;

  *local_address = NULL;
  *broadcast_address = NULL;

  bool ret = execute_query( db, "select reflectors.broadcast_address,reflectors.broadcast_port from "
                            "overlay_networks,reflectors where overlay_networks.reflector_group_id = reflectors.group_id and "
                            "overlay_networks.slice_id = %u", vni );
  if ( !ret ) {
    goto error;
  }

  result = mysql_store_result( db );
  if ( result == NULL ) {
    error( "Failed to retrieve result from database ( %s ).", mysql_error( db ) );
    goto error;
  }

  assert( mysql_num_fields( result ) == 2 );
  if ( mysql_num_rows( result ) == 0 ) {
    warn( "Failed to retrieve overlay network configuration ( vni = %#x ).", vni );
    goto error;
  }

  MYSQL_ROW row = mysql_fetch_row( result );
  if ( row != NULL ) {
    *broadcast_address = xstrdup( row[ 0 ] );
    ret = string_to_uint16( row[ 1 ], broadcast_port );
    if ( !ret ) {
      error( "Invalid transport port number ( %s ).", row[ 1 ] );
      goto error;
    }
  }
  else {
    error( "Failed to fetch data." );
    goto error;
  }

  mysql_free_result( result );
  result = NULL;

  ret = execute_query( db, "select local_address,local_port from tunnel_endpoints where datapath_id = %" PRIu64, datapath_id );
  if ( !ret ) {
    goto error;
  }

  result = mysql_store_result( db );
  if ( result == NULL ) {
    error( "Failed to retrieve result from database ( %s ).", mysql_error( db ) );
    goto error;
  }

  assert( mysql_num_fields( result ) == 2 );
  if ( mysql_num_rows( result ) == 0 ) {
    warn( "Failed to retrieve tunnel endpoint configuration ( datapath_id = %#" PRIx64 " ).", datapath_id );
    goto error;
  }

  row = mysql_fetch_row( result );
  if ( row != NULL ) {
    *local_address = xstrdup( row[ 0 ] );
    ret = string_to_uint16( row[ 1 ], local_port );
    if ( !ret ) {
      error( "Invalid transport port number ( %s ).", row[ 1 ] );
      goto error;
    }
  }
  else {
    error( "Failed to fetch data." );
    goto error;
  }

  mysql_free_result( result );

  debug( "Overlay network information is retrieved ( vni = %#x, datapath_id = %#" PRIx64 ", local_address = %s, local_port = %u, "
         "broadcast_address = %s, broadcast_port = %u ).", vni, datapath_id, *local_address, *local_port, *broadcast_address, *broadcast_port );

  return true;

error:
  error( "Failed to retrieve overlay network information from database ( vni = %#x, datapath_id = %#" PRIx64 ", local_address = %p, local_port = %p, "
         "broadcast_address = %p, broadcast_port = %p ).", vni, datapath_id, local_address, local_port, broadcast_address, broadcast_port );

  if ( result != NULL ) {
    mysql_free_result( result );
  }
  if ( *broadcast_address != NULL ) {
    xfree( *broadcast_address );
    *broadcast_address = NULL;
  }
  *broadcast_port = 0;
  if ( *local_address != NULL ) {
    xfree( *local_address );
    *local_address = NULL;
  }
  *local_port = 0;

  return false;
}


static bool
get_reflectors_to_update( list_element **reflectors_to_update, uint32_t vni ) {
  debug( "Retriving packet reflectors to update ( reflectors_to_update = %p, vni = %#x ).", reflectors_to_update, vni );

  assert( db != NULL );
  assert( reflectors_to_update != NULL );

  MYSQL_RES *result = NULL;

  bool ret = execute_query( db, "select reflectors.uri from reflectors,overlay_networks "
                            "where overlay_networks.reflector_group_id = reflectors.group_id and "
                            "overlay_networks.slice_id = %u", vni );
  if ( !ret ) {
    return false;
  }

  result = mysql_store_result( db );
  if ( result == NULL ) {
    error( "Failed to retrieve result from database ( %s ).", mysql_error( db ) );
    return false;
  }

  assert( mysql_num_fields( result ) == 1 );
  if ( mysql_num_rows( result ) == 0 ) {
    error( "Failed to retrieve packet reflector configuration ( vni = %#x ).", vni );
    mysql_free_result( result );
    return false;
  }

  create_list( reflectors_to_update );

  char *uri = NULL;
  MYSQL_ROW row;
  while ( ( row = mysql_fetch_row( result ) ) != NULL ) {
    uri = xstrdup( row[ 0 ] );
    append_to_tail( reflectors_to_update, uri );
  }

  if ( list_length_of( *reflectors_to_update ) == 0 ) {
    delete_list( *reflectors_to_update );
  }

  mysql_free_result( result );

  return true;
}


static bool
get_agent_uri( uint64_t datapath_id, char **uri ) {
  debug( "Retrieving agent's URI ( datapath_id = %#" PRIx64 ", uri = %p ).", datapath_id, uri );

  assert( uri != NULL );

  MYSQL_RES *result = NULL;

  *uri = NULL;

  bool ret = execute_query( db, "select uri from agents where datapath_id = %" PRIu64, datapath_id );
  if ( !ret ) {
    goto error;
  }

  result = mysql_store_result( db );
  if ( result == NULL ) {
    error( "Failed to retrieve result from database ( %s ).", mysql_error( db ) );
    goto error;
  }

  assert( mysql_num_fields( result ) == 1 );
  if ( mysql_num_rows( result ) == 0 ) {
    warn( "Failed to retrieve agent information ( datapath_id = %#" PRIx64 " ).", datapath_id );
    goto error;
  }

  MYSQL_ROW row = mysql_fetch_row( result );
  if ( row != NULL ) {
    *uri = xstrdup( row[ 0 ] );
  }
  else {
    error( "Failed to fetch data." );
    goto error;
  }

  mysql_free_result( result );

  debug( "Agent's URI is retrieved ( datapath_id = %#" PRIx64 ", uri = %s ).", datapath_id, *uri );

  return true;

error:
  error( "Failed to retrieve agent's URI ( datapath_id = %#" PRIx64 ", uri = %p ).", datapath_id, uri );

  if ( result != NULL ) {
    mysql_free_result( result );
  }
  if ( *uri != NULL ) {
    xfree( *uri );
    *uri = NULL;
  }

  return false;
}


static void
wait_for_port_to_be_added( void *user_data ) {
  transaction_entry *entry = user_data;

  debug( "Waiting for a tunnel port to be added ( entry = %p, datapath_id = %#" PRIx64 ", vni = %#x ).",
         entry, entry != NULL ? entry->datapath_id : 0, entry != NULL ? entry->vni : 0 );

  assert( user_data != NULL );

  uint16_t port_no = 0;
  char name[ IFNAMSIZ ];
  memset( name, '\0', sizeof( name ) );
  snprintf( name, sizeof( name ), "vxlan%u", entry->vni & VNI_MASK );
  bool ret = get_port_no_by_name( entry->datapath_id, name, &port_no );
  if ( !ret ) {
    if ( entry->port_wait_count >= 10 ) {
      warn( "A tunnel port is not added to a switch ( datapath_id = %#" PRIx64 ", vni = %#x, name = %s ).",
            entry->datapath_id, entry->vni, name );
      if ( entry->callback != NULL ) {
        debug( "Calling a callback function ( calback = %p, user_data = %p ).", entry->callback, entry->user_data );
        entry->callback( OVERLAY_NETWORK_OPERATION_FAILED, entry->user_data );
      }
      delete_timer_event( wait_for_port_to_be_added, entry );
      delete_transaction( entry->datapath_id, entry->vni );
    }
    else {
      entry->port_wait_count++;
      debug( "port_wait_count = %d.", entry->port_wait_count );
    }
    return;
  }

  debug( "A tunnel port is successfully added to a switch ( datapath_id = %#" PRIx64 ", vni = %#x, name = %s, port_no = %u ).",
         entry->datapath_id, entry->vni, name, port_no );
  if ( entry->callback != NULL ) {
    debug( "Calling a callback function ( calback = %p, user_data = %p ).", entry->callback, entry->user_data );
    entry->callback( OVERLAY_NETWORK_OPERATION_SUCCEEDED, entry->user_data );
  }
  delete_timer_event( wait_for_port_to_be_added, entry );
  delete_transaction( entry->datapath_id, entry->vni );
}


static void
wait_for_port_to_be_deleted( void *user_data ) {
  transaction_entry *entry = user_data;

  debug( "Waiting for a tunnel port to be deleted ( entry = %p, datapath_id = %#" PRIx64 ", vni = %#x ).",
         entry, entry != NULL ? entry->datapath_id : 0, entry != NULL ? entry->vni : 0 );

  assert( user_data != NULL );

  uint16_t port_no = 0;
  char name[ IFNAMSIZ ];
  memset( name, '\0', sizeof( name ) );
  snprintf( name, sizeof( name ), "vxlan%u", entry->vni & VNI_MASK );
  bool ret = get_port_no_by_name( entry->datapath_id, name, &port_no );
  if ( ret ) {
    if ( entry->port_wait_count >= 10 ) {
      warn( "A tunnel port is not deleted from a switch ( datapath_id = %#" PRIx64 ", vni = %#x, name = %s ).",
            entry->datapath_id, entry->vni, name );
      if ( entry->callback != NULL ) {
        debug( "Calling a callback function ( calback = %p, user_data = %p ).", entry->callback, entry->user_data );
        entry->callback( OVERLAY_NETWORK_OPERATION_FAILED, entry->user_data );
      }
      delete_timer_event( wait_for_port_to_be_deleted, entry );
      delete_transaction( entry->datapath_id, entry->vni );
    }
    else {
      entry->port_wait_count++;
      debug( "port_wait_count = %d.", entry->port_wait_count );
    }
    return;
  }

  debug( "A tunnel port is successfully deleted from a switch ( datapath_id = %#" PRIx64 ", vni = %#x, name = %s, port_no = %u ).",
         entry->datapath_id, entry->vni, name, port_no );
  if ( entry->callback != NULL ) {
    debug( "Calling a callback function ( calback = %p, user_data = %p ).", entry->callback, entry->user_data );
    entry->callback( OVERLAY_NETWORK_OPERATION_SUCCEEDED, entry->user_data );
  }
  delete_timer_event( wait_for_port_to_be_deleted, entry );
  delete_transaction( entry->datapath_id, entry->vni );
}


static void
http_transaction_completed( int status, int code, const http_content *content, void *user_data ) {
  debug( "A HTTP transaction completed ( status = %d, code = %d, content = %p, user_data = %p ).",
         status, code, content, user_data );

  assert( user_data != NULL );

  transaction_entry *entry = user_data;

  entry->n_ongoing_http_requests--;

  if ( status == HTTP_TRANSACTION_FAILED ) {
    if ( !( ( entry->operation == OPERATION_ATTACH ) && ( code == 440 ) ) &&   // 440: Specified TEP has already been added.
         !( ( entry->operation == OPERATION_DETACH ) && ( code == 404 ) ) ) {  // 404: Already deleted.
      error( "HTTP transaction failed ( vni = %#x, datapath_id = %#" PRIx64 ", operation = %#x, port_wait_count = %d, "
             "n_ongoing_http_requests = %u, n_failed_http_requests = %u, status = %d, code = %d ).",
             entry->vni, entry->datapath_id, entry->operation, entry->port_wait_count,
             entry->n_ongoing_http_requests, entry->n_failed_http_requests, status, code );
      entry->n_failed_http_requests++;
    }
  }

  if ( entry->n_ongoing_http_requests > 0 ) {
    return;
  }

  assert( entry->n_ongoing_http_requests == 0 );

  if ( entry->n_failed_http_requests == 0 ) {
    if ( entry->callback != NULL ) {
      if ( entry->operation == OPERATION_ATTACH ) {
        add_periodic_event_callback( 1, wait_for_port_to_be_added, entry );
        return;
      }
      else if ( entry->operation == OPERATION_DETACH ) {
        add_periodic_event_callback( 1, wait_for_port_to_be_deleted, entry );
        return;
      }
    }
  }
  else {
    if ( entry->callback != NULL ) {
      debug( "Calling a callback function ( calback = %p, user_data = %p ).", entry->callback, entry->user_data );
      entry->callback( OVERLAY_NETWORK_OPERATION_FAILED, entry->user_data );
    }
  }

  delete_transaction( entry->datapath_id, entry->vni );
}


static bool
add_tep_to_packet_reflectors( transaction_entry *entry, uint32_t vni, const char *address, uint16_t port ) {
  debug( "Adding a tunnel endpoint to packet reflectors ( entry = %p, vni = %#x, address = %s, port = %u ).",
         entry, vni, address != NULL ? address : "", port );

  assert( entry != NULL );
  assert( address != NULL );
  assert( port > 0 );

  if ( !valid_vni( vni ) ) {
    return false;
  }

  char *post_uri = NULL;
  char *json_string = NULL;
  list_element *reflectors = NULL;
  http_content content;
  memset( &content, 0, sizeof( http_content ) );

  bool ret = get_reflectors_to_update( &reflectors, vni );
  if ( !ret ) {
    error( "Failed to retrieve reflectors information ( vni = %#x ).", vni );
    goto error;
  }

  ret = create_json_to_add_tep( &json_string, address, port );
  if ( !ret || json_string == NULL || ( json_string != NULL && strlen( json_string ) == 0 ) ) {
    error( "Failed to create json string for packet reflector." );
    goto error;
  }

  memset( content.content_type, '\0', sizeof( content.content_type ) );
  snprintf( content.content_type, sizeof( content.content_type ), "application/json" );

  size_t json_string_length = strlen( json_string ) + 1;
  content.body = alloc_buffer_with_length( json_string_length );
  void *p = append_back_buffer( content.body, json_string_length );
  memcpy( p, json_string, json_string_length );

  for ( list_element *e = reflectors; e != NULL; e = e->next ) {
    char *base_uri = e->data;
    size_t post_uri_length = strlen( base_uri ) + strlen( "reflector/" ) + MAX_VNI_STRLEN;
    post_uri = xmalloc( post_uri_length );
    memset( post_uri, '\0', post_uri_length );
    snprintf( post_uri, post_uri_length, "%sreflector/%u", base_uri, vni & VNI_MASK );
    ret = do_http_request( HTTP_METHOD_POST, post_uri, &content, http_transaction_completed, entry );
    if ( !ret ) {
      error( "Failed to send a HTTP request." );
      entry->n_failed_http_requests++;
      goto error;
    }
    entry->n_ongoing_http_requests++;
    xfree( post_uri );
  }

  free_list( reflectors );
  xfree( json_string );
  free_buffer( content.body );

  return true;

error:
  if ( reflectors != NULL ) {
    free_list( reflectors );
  }
  if ( json_string != NULL ) {
    xfree( json_string );
  }
  if ( post_uri != NULL ) {
    xfree( post_uri );
  }
  if ( content.body != NULL ) {
    free_buffer( content.body );
  }

  return false;
}


static bool
delete_tep_from_packet_reflectors( transaction_entry *entry, uint32_t vni, const char *address ) {
  debug( "Deleting a tunnel endpoint from packet reflectors ( entry = %p, vni = %#x, address = %s ).",
         entry, vni, address != NULL ? address : "" );

  assert( entry != NULL );
  assert( address != NULL );

  if ( !valid_vni( vni ) ) {
    return false;
  }

  char *delete_uri = NULL;
  list_element *reflectors = NULL;
  
  bool ret = get_reflectors_to_update( &reflectors, vni );
  if ( !ret ) {
    error( "Failed to retrieve reflectors information ( vni = %#x ).", vni );
    goto error;
  }

  for ( list_element *e = reflectors; e != NULL; e = e->next ) {
    char *base_uri = e->data;
    size_t delete_uri_length = strlen( base_uri ) + strlen( "reflector/" ) + MAX_VNI_STRLEN  + INET_ADDRSTRLEN;
    delete_uri = xmalloc( delete_uri_length );
    memset( delete_uri, '\0', delete_uri_length );
    snprintf( delete_uri, delete_uri_length, "%sreflector/%u/%s", base_uri, vni & VNI_MASK, address );
    ret = do_http_request( HTTP_METHOD_DELETE, delete_uri, NULL, http_transaction_completed, entry );
    if ( !ret ) {
      error( "Failed to send a HTTP request." );
      entry->n_failed_http_requests++;
      goto error;
    }
    entry->n_ongoing_http_requests++;
    xfree( delete_uri );
  }

  free_list( reflectors );

  return true;

error:
  if ( reflectors != NULL ) {
    free_list( reflectors );
  }
  if ( delete_uri != NULL ) {
    xfree( delete_uri );
  }

  return false;
}


static bool
add_tunnel_to_tep( transaction_entry *entry, uint32_t vni, uint64_t datapath_id, const char *broadcast_address, uint16_t broadcast_port ) {
  debug( "Adding a virtual network to a tunnel endpoint ( entry = %p, vni = %#x, datapath_id = %#" PRIx64 ", broadcast_address = %s, broadcast_port = %u ).",
         entry, vni, datapath_id, broadcast_address != NULL ? broadcast_address : "", broadcast_port );

  assert( entry != NULL );
  assert( broadcast_address != NULL );

  char *base_uri = NULL;
  char *post_uri = NULL;
  char *json_string = NULL;
  http_content content;

  memset( &content, 0 , sizeof( http_content ) );

  bool ret = get_agent_uri( datapath_id, &base_uri );
  if ( !ret ) {
    error( "Failed to retrieve agent information ( datapath_id = %#" PRIx64 " ).", datapath_id );
    goto error;
  }

  ret = create_json_to_add_tunnel( &json_string, vni, broadcast_address, broadcast_port );
  if ( !ret || json_string == NULL || ( json_string != NULL && strlen( json_string ) == 0 ) ) {
    error( "Failed to create json string for Gate Switch." );
    goto error;
  }

  size_t post_uri_length = strlen( base_uri ) + strlen( "overlay_networks" ) + 1;
  post_uri = xmalloc( post_uri_length );
  memset( post_uri, '\0', post_uri_length );
  snprintf( post_uri, post_uri_length, "%soverlay_networks", base_uri );

  memset( content.content_type, '\0', sizeof( content.content_type ) );
  snprintf( content.content_type, sizeof( content.content_type ), "application/json" );
  size_t json_string_length = strlen( json_string ) + 1;
  content.body = alloc_buffer_with_length( json_string_length );
  void *p = append_back_buffer( content.body, json_string_length );
  memcpy( p, json_string, json_string_length );
  ret = do_http_request( HTTP_METHOD_POST, post_uri, &content, http_transaction_completed, entry );
  if ( !ret ) {
    error( "Failed to send a HTTP request to HTTP client thread ( datapath_id = %" PRIx64 " ).", datapath_id );
    entry->n_failed_http_requests++;
    goto error;
  }
  entry->n_ongoing_http_requests++;

  xfree( base_uri );
  xfree( post_uri );
  xfree( json_string );
  free_buffer( content.body );

  return true;

error:
  if ( base_uri != NULL ) {
    xfree( base_uri );
  }
  if ( post_uri != NULL ) {
    xfree( post_uri );
  }
  if ( json_string != NULL) {
    xfree( json_string );
  }
  if ( content.body != NULL ) {
    free_buffer( content.body );
  }

  return false;
}


static bool
delete_tunnel_from_tep( transaction_entry *entry, uint32_t vni, uint64_t datapath_id ) {
  debug( "Deleting a virtual network from a tunnel endpoint ( entry = %p, vni = %#x, datapath_id = %#" PRIx64 ").", entry, vni, datapath_id );

  assert( entry != NULL );

  char *base_uri = NULL;
  char *delete_uri = NULL;

  bool ret = get_agent_uri( datapath_id, &base_uri );
  if ( !ret ) {
    error( "Failed to retrieve agent information ( datapath_id = %#" PRIx64 " ).", datapath_id );
    goto error;
  }

  size_t delete_uri_length = strlen( base_uri ) + strlen( "overlay_networks/" ) + MAX_VNI_STRLEN;
  delete_uri = xmalloc( delete_uri_length );
  memset( delete_uri, '\0', delete_uri_length );
  snprintf( delete_uri, delete_uri_length, "%soverlay_networks/%u", base_uri, vni & VNI_MASK );

  ret = do_http_request( HTTP_METHOD_DELETE, delete_uri, NULL, http_transaction_completed, entry );
  if ( !ret ) {
    error( "Failed to send a HTTP request to HTTP client thread ( datapath_id = %" PRIx64 " ).", datapath_id );
    entry->n_failed_http_requests++;
    goto error;
  }
  entry->n_ongoing_http_requests++;

  xfree( base_uri );
  xfree( delete_uri );

  return true;

error:
  if ( base_uri != NULL ) {
    xfree( base_uri );
  }
  if ( delete_uri != NULL ) {
    xfree( delete_uri );
  }

  return false;
}


int
attach_to_network( uint64_t datapath_id, uint32_t vni, overlay_operation_completed_handler callback, void *user_data ) {
  debug( "Attaching a switch %#" PRIx64 " to an overlay network ( vni = %#x, callback = %p, user_data = %p ).",
         datapath_id, vni, callback, user_data );

  char *local_address = NULL;
  char *broadcast_address = NULL;
  uint16_t local_port = 0;
  uint16_t broadcast_port = 0;

  if ( !valid_vni( vni ) ) {
    return OVERLAY_NETWORK_OPERATION_FAILED;
  }

  transaction_entry *entry = lookup_transaction( datapath_id, vni );
  if ( entry != NULL ) {
    if ( entry->datapath_id == datapath_id && entry->vni == vni &&
         entry->operation == OPERATION_ATTACH ) {
      debug( "Same operation is in progress ( datapath_id = %#" PRIx64 ", vni = %#x, operation = %#x ).",
             entry->datapath_id, entry->vni, entry->operation );
      return OVERLAY_NETWORK_OPERATION_IN_PROGRESS;
    }
    error( "Another operation is in progress ( datapath_id = %#" PRIx64 ", vni = %#x, operation = %#x ).",
           entry->datapath_id, entry->vni, entry->operation );
    return OVERLAY_NETWORK_OPERATION_FAILED;
  }

  bool ret = add_transaction( datapath_id, vni, OPERATION_ATTACH, callback, user_data );
  if ( !ret ) {
    error( "Failed to add a transaction for attaching to a network "
           "( datapath_id = %" PRIx64 ", vni = %#x, callback = %p, user_data = %p ).",
           datapath_id, vni, callback, user_data );
    return OVERLAY_NETWORK_OPERATION_FAILED;
  }

  ret = get_overlay_info( vni, datapath_id, &local_address, &local_port, &broadcast_address, &broadcast_port );
  if ( !ret ) {
    error( "Failed to retrieve overlay network information ( datapath_id = %#" PRIx64 ", vni = %#x ).",
           datapath_id, vni );
    goto error;
  }

  entry = lookup_transaction( datapath_id, vni );
  if ( entry == NULL ) {
    error( "Failed to retrieve transaction entry ( datapath_id = %#" PRIx64 ", vni = %#x ).",
           datapath_id, vni );
    goto error;
  }

  if ( !valid_multicast_address( broadcast_address ) ) {
    ret = add_tep_to_packet_reflectors( entry, vni, local_address, local_port );
    if ( !ret ) {
      if ( entry->n_ongoing_http_requests == 0 ) {
        error( "Failed to add TEP to all Packet Reflectors." );
        goto error;
      }
      error( "Failed to add TEP to some Packet Reflectors." );
    }
  }

  if ( ret ) {
    ret = add_tunnel_to_tep( entry, vni, datapath_id, broadcast_address, broadcast_port );
    if ( !ret ) {
      if ( entry->n_ongoing_http_requests == 0 ) {
        error( "Failed to create tunnel." );
        goto error;
      }
      error( "Failed to create a tunnel. But http request to create tunnel is ongoing." );
    }
  }

  xfree( broadcast_address );
  xfree( local_address );

  debug( "A HTTP request is sent to HTTP client thread ( datapath_id = %" PRIx64 ", vni = %#x, callback = %p, user_data = %p ).",
         datapath_id, vni, callback, user_data );

  return OVERLAY_NETWORK_OPERATION_SUCCEEDED;

error:
  delete_transaction( datapath_id, vni );

  if ( broadcast_address != NULL ) {
    xfree( broadcast_address );
  }
  if ( local_address != NULL ) {
    xfree( local_address );
  }

  return OVERLAY_NETWORK_OPERATION_FAILED;
}


int
detach_from_network( uint64_t datapath_id, uint32_t vni,
                     overlay_operation_completed_handler callback, void *user_data ) {
  debug( "Detaching a switch %#" PRIx64 " from an overlay network ( vni = %#x, callback = %p, user_data = %p ).",
         datapath_id, vni, callback, user_data );

  char *local_address = NULL;
  char *broadcast_address = NULL;
  uint16_t local_port = 0;
  uint16_t broadcast_port = 0;

  if ( !valid_vni( vni ) ) {
    return OVERLAY_NETWORK_OPERATION_FAILED;
  }

  transaction_entry *entry = lookup_transaction( datapath_id, vni );
  if ( entry != NULL ) {
    if ( entry->datapath_id == datapath_id && entry->vni == vni &&
         entry->operation == OPERATION_DETACH ) {
      debug( "Same operation is in progress ( datapath_id = %#" PRIx64 ", vni = %#x, operation = %#x ).",
             entry->datapath_id, entry->vni, entry->operation );
      return OVERLAY_NETWORK_OPERATION_IN_PROGRESS;
    }
    error( "Another operation is in progress ( datapath_id = %#" PRIx64 ", vni = %#x, operation = %#x ).",
           entry->datapath_id, entry->vni, entry->operation );
    return OVERLAY_NETWORK_OPERATION_FAILED;
  }

  bool ret = add_transaction( datapath_id, vni, OPERATION_DETACH, callback, user_data );
  if ( !ret ) {
    error( "Failed to add a transaction for detaching from a network "
           "( datapath_id = %" PRIx64 ", vni = %#x, callback = %p, user_data = %p ).",
           datapath_id, vni, callback, user_data );
    return OVERLAY_NETWORK_OPERATION_FAILED;
  }

  ret = get_overlay_info( vni, datapath_id, &local_address, &local_port, &broadcast_address, &broadcast_port );
  if ( !ret ) {
    error( "Failed to retrieve overlay network information ( datapath_id = %#" PRIx64 ", vni = %#x ).",
           datapath_id, vni );
    goto error;
  }

  entry = lookup_transaction( datapath_id, vni );
  if ( entry == NULL ) {
    error( "Failed to retrieve transaction entry ( datapath_id = %#" PRIx64 ", vni = %#x ).",
           datapath_id, vni );
    goto error;
  }

  ret = delete_tunnel_from_tep( entry, vni, datapath_id );
  if ( !ret ) {
    if ( entry->n_ongoing_http_requests == 0 ) {
      error( "Failed to delete a tunnel." );
      goto error;
    }
    error( "Failed to delete a tunnel, but http request to delete a tunnel is ongoing." );
  }

  if ( !valid_multicast_address( broadcast_address ) ) {
    if ( ret ) {
      ret = delete_tep_from_packet_reflectors( entry, vni, local_address );
      if ( !ret ) {
        if ( entry->n_ongoing_http_requests == 0 ) {
          error( "Failed to delete TEP from all Packet Reflectors." );
          goto error;
        }
        error( "Failed to delete TEP from some Packet Reflectors." );
      }
    }
  }

  xfree( broadcast_address );
  xfree( local_address );

  debug( "A HTTP request is sent to HTTP client thread ( datapath_id = %" PRIx64 ", vni = %#x, callback = %p, user_data = %p ).",
         datapath_id, vni, callback, user_data );

  return OVERLAY_NETWORK_OPERATION_SUCCEEDED;

error:
  delete_transaction( datapath_id, vni );

  if ( broadcast_address != NULL ) {
    xfree( broadcast_address );
  }
  if ( local_address != NULL ) {
    xfree( local_address );
  }

  return OVERLAY_NETWORK_OPERATION_FAILED;
}


bool
init_overlay_network_manager( MYSQL *_db ) {
  debug( "Initializing overlay network manager ( db = %p ).", _db );

  if ( _db == NULL ) {
    error( "Failed to initialize overlay network manager. MySQL handle must not be NULL." );
    return false;
  }

  db = _db;

  bool ret = create_transaction_db();
  if ( !ret ) {
    error( "Failed to initialize overlay network manager." );
    return false;
  }

  debug( "Initialization completed." );

  return true;
}


bool
finalize_overlay_network_manager() {
  debug( "Finalizing overlay network manager ( db = %p ).", db );

  if ( db == NULL ) {
    error( "Failed to finalize overlay network manager. MySQL handle is NULL." );
    return false;
  }

  db = NULL;

  bool ret = delete_transaction_db();
  if ( !ret ) {
    error( "Failed to finalize overlay network manager." );
    return false;
  }

  debug( "Finalization completed." );

  return true;
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
