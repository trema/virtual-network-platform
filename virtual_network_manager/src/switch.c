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
#include <linux/limits.h>
#include <mysql/mysql.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "db.h"
#include "switch.h"
#ifdef UNIT_TEST
#include "switch_unittest.h"
#endif


static MYSQL *db = NULL;
static switch_port_event_handler port_event_callback = NULL;
static void *port_event_user_data = NULL;


bool
add_port( uint64_t datapath_id, uint16_t port_no, const char *name ) {
  debug( "Adding a switch port to port database ( datapath_id = %#" PRIx64 ", port_no = %u, name = %s ).",
         datapath_id, port_no, name != NULL ? name : "" );

  assert( db != NULL );
  assert( name != NULL );

  bool ret = execute_query( db, "replace into switch_ports (datapath_id,port_no,name) values (%" PRIu64 ",%u,\'%s\')",
                            datapath_id, port_no, name );
  if ( !ret ) {
    return false;
  }

  my_ulonglong n_rows = mysql_affected_rows( db );
  if ( n_rows == 0 ) {
    error( "Failed to insert a port to database ( datapath_id = %#" PRIx64 ", port_no = %u, name = %s ).",
           datapath_id, port_no, name );
    return false;
  }

  if ( port_event_callback != NULL ) {
    debug( "Calling port event callback ( callback = %p, event = %#x, datapath_id = %#" PRIx64 ", "
           "name = %s, port_no = %u, user_data = %p ).",
           port_event_callback, SWITCH_PORT_ADDED, datapath_id, name, port_no, port_event_user_data );
    port_event_callback( SWITCH_PORT_ADDED, datapath_id, name, port_no, port_event_user_data );
  }

  debug( "Switch port is added to port database ( datapath_id = %#" PRIx64 ", port_no = %u, name = %s ).",
         datapath_id, port_no, name );

  return true;
}


bool
delete_port( uint64_t datapath_id, uint16_t port_no, const char *name ) {
  debug( "Deleting a port from port database ( datapath_id = %#" PRIx64 ", port_no = %u, name = %s ).",
         datapath_id, port_no, name != NULL ? name : "" );

  assert( db != NULL );

  bool ret = false;
  if ( name != NULL ) {
    ret = execute_query( db, "delete from switch_ports where datapath_id = %" PRIu64 " and port_no = %u and name = \'%s\'",
                         datapath_id, port_no, name );
  }
  else {
    ret = execute_query( db, "delete from switch_ports where datapath_id = %" PRIu64 " and port_no = %u", datapath_id, port_no );
  }
  if ( !ret ) {
    return false;
  }

  my_ulonglong n_rows = mysql_affected_rows( db );
  if ( n_rows == 0 ) {
    warn( "Failed to delete a port from database ( datapath_id = %#" PRIx64 ", port_no = %u, name = %s ).",
          datapath_id, port_no, name != NULL ? name : "" );
    return false;
  }

  if ( port_event_callback != NULL ) {
    debug( "Calling port event callback ( callback = %p, event = %#x, datapath_id = %#" PRIx64 ", "
           "name = %s, port_no = %u, user_data = %p ).",
           port_event_callback, SWITCH_PORT_DELETED, datapath_id, name != NULL ? name : "", port_no, port_event_user_data );
    port_event_callback( SWITCH_PORT_DELETED, datapath_id, name, port_no, port_event_user_data );
  }

  debug( "Switch port is deleted from port database ( datapath_id = %#" PRIx64 ", port_no = %u, name = %s ).",
         datapath_id, port_no, name != NULL ? name : "" );

  return true;
}


bool
get_port_no_by_name( uint64_t datapath_id, const char *name, uint16_t *port_no ) {
  debug( "Looking up a port number by name ( datapath_id = %#" PRIx64 ", name = %s, port_no = %p ).",
         datapath_id, name != NULL ? name : "", port_no );

  assert( db != NULL );
  assert( name != NULL );
  assert( port_no != NULL );

  *port_no = OFPP_NONE;

  bool ret = execute_query( db, "select port_no from switch_ports where datapath_id = %" PRIu64 " and name = \'%s\'",
                            datapath_id, name );
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
    debug( "Port not found ( name = %s ).", name );
    mysql_free_result( result );
    return false;
  }

  MYSQL_ROW row = mysql_fetch_row( result );
  assert( row != NULL );
  char *endp = NULL;
  *port_no = ( uint16_t ) strtoul( row[ 0 ], &endp, 0 );
  if ( *endp != '\0' ) {
    error( "Invalid port number ( %s ).", row[ 0 ] );
    *port_no = OFPP_NONE;
    mysql_free_result( result );
    return false;
  }

  mysql_free_result( result );

  debug( "Port number found ( datapath_id = %#" PRIx64 ", name = %s, port_no = %u ).",
         datapath_id, name, *port_no );

  return true;
}


static bool
delete_ports_by_datapath_id( uint64_t datapath_id, const char *controller_host, pid_t controller_pid ) {
  debug( "Deleting switch ports from port database ( datapath_id = %#" PRIx64 ", "
         "controller_host = %s, controller_pid = %u ).", datapath_id, controller_host, controller_pid );

  assert( db != NULL );

  bool ret = execute_query( db, "delete from switch_ports where datapath_id in (select datapath_id from "
                            "switches where datapath_id = %" PRIu64 " and controller_host = \'%s\' and "
                            "controller_pid = %u)", datapath_id, controller_host, controller_pid );
  if ( !ret ) {
    return false;
  }

  my_ulonglong n_rows = mysql_affected_rows( db );
  if ( n_rows == 0 ) {
    warn( "Failed to delete ports from database ( datapath_id = %#" PRIx64 ", "
          "controller_host = %s, controller_pid = %u ).", datapath_id, controller_host, controller_pid );
    return false;
  }

  if ( port_event_callback != NULL ) {
    debug( "Calling port event callback ( callback = %p, event = %#x, datapath_id = %#" PRIx64 ", "
           "port_no = %u, user_data = %p ).",
           port_event_callback, SWITCH_PORT_DELETED, datapath_id, OFPP_ALL, port_event_user_data );
    port_event_callback( SWITCH_PORT_DELETED, datapath_id, NULL, OFPP_ALL, port_event_user_data );
  }

  debug( "Switch ports are deleted from port database successfully ( datapath_id = %#" PRIx64 ", "
         "controller_host = %s, controller_pid = %u ).", datapath_id, controller_host, controller_pid );

  return true;
}


bool
add_switch( uint64_t datapath_id, const char *controller_host, pid_t controller_pid ) {
  debug( "Adding a switch %#" PRIx64 " to switch database ( controller_host = %s, controller_pid = %u ).",
         datapath_id, controller_host, controller_pid );

  assert( db != NULL );

  bool ret = execute_query( db, "replace into switches (datapath_id,controller_host,controller_pid) values (%" PRIu64 ",\'%s\',%u)",
                            datapath_id, controller_host, controller_pid );
  if ( !ret ) {
    return false;
  }

  my_ulonglong n_rows = mysql_affected_rows( db );
  if ( n_rows == 0 ) {
    error( "Failed to add a switch to database ( datapath_id = %#" PRIx64 ", "
           "controller_host = %s, controller_pid = %u ).", datapath_id, controller_host, controller_pid );
    return false;
  }

  debug( "Switch %#" PRIx64 " is added into switch database successfully ( controller_host = %s, controller_pid = %u ).",
         datapath_id, controller_host, controller_pid );

  return true;
}


bool
delete_switch( uint64_t datapath_id, const char *controller_host, pid_t controller_pid ) {
  debug( "Deleting a switch %#" PRIx64 " ( controller_host = %s, controller_pid = %u ) "
         "from switch database.", datapath_id, controller_host, controller_pid );

  assert( db != NULL );

  int n_errors = 0;
  bool ret = delete_ports_by_datapath_id( datapath_id, controller_host, controller_pid );
  if ( !ret ) {
    warn( "Failed to delete switch ports from database ( datapath_id = %#" PRIx64 ", "
          "controller_host = %s, controller_pid = %u ).", datapath_id, controller_host, controller_pid );
    n_errors++;
  }

  ret = execute_query( db, "delete from switches where datapath_id = %" PRIu64 " and "
                       "controller_host = \'%s\' and controller_pid = %u",
                       datapath_id, controller_host, controller_pid );

  if ( !ret ) {
    warn( "Failed to delete a switch from database ( datapath_id = %#" PRIx64 ", "
          "controller_host = %s, controller_pid = %u ).", datapath_id, controller_host, controller_pid );
    n_errors++;
  }

  if ( n_errors > 0 ) {
    return false;
  }

  my_ulonglong n_rows = mysql_affected_rows( db );
  if ( n_rows == 0 ) {
    warn( "Failed to delete a switch from database ( datapath_id = %#" PRIx64 ", "
          "controller_host = %s, controller_pid = %u ).", datapath_id, controller_host, controller_pid );
    return false;
  }

  debug( "Switch %#" PRIx64 " is deleted from switch database successfully.", datapath_id );

  return true;
}


bool
delete_switch_by_host_pid( const char* controller_host, pid_t controller_pid ) {
  debug( "Deleting switches from switch database ( controller_host = %s, controller_pid = %u ).",
         controller_host, controller_pid );

  assert( db != NULL );

  bool ret = execute_query( db, "select datapath_id from switches where controller_host = \'%s\' and "
                            "controller_pid = %u", controller_host, controller_pid );
  if ( !ret ) {
    return false;
  }

  MYSQL_RES *result = mysql_store_result( db );
  if ( result == NULL ) {
    error( "Failed to retrieve result from database ( %s ).", mysql_error( db ) );
    return false;
  }

  my_ulonglong n_rows = mysql_num_rows( result );
  if ( n_rows == 0 ) {
    debug( "Nothing to delete." );
    mysql_free_result( result );
    return true;
  }

  debug( "Deleting %u switches.", ( unsigned int ) n_rows );

  assert( mysql_num_fields( result ) == 1 );

  int n_errors = 0;
  MYSQL_ROW row;
  while ( ( row = mysql_fetch_row( result ) ) != NULL ) {
    char *endp = NULL;
    uint64_t datapath_id = ( uint64_t ) strtoull( row[ 0 ], &endp, 0 );
    if ( *endp != '\0' ) {
      error( "Invalid datapath_id ( %s ).", row[ 0 ] );
      n_errors++;
      continue;
    }

    bool ret = delete_switch( datapath_id, controller_host, controller_pid );
    if ( !ret ) {
      n_errors++;
    }
  }

  mysql_free_result( result );

  ret = ( n_errors == 0 ) ? true : false;
  if ( !ret ) {
    debug( "Failed to delete switches from switch database ( controller_host = %s, controller_pid = %u ).",
           controller_host, controller_pid );
    return false;
  }

  return true;
}


bool
switch_on_duty( uint64_t datapath_id, const char *controller_host, pid_t controller_pid ) {
  debug( "Checking if switch %#" PRIx64 " is on our duty or not ( controller_host = %p, controller_pid = %u ).",
         datapath_id, controller_host, controller_pid );

  assert( db != NULL );

  bool ret = execute_query( db, "select datapath_id from switches where datapath_id = %" PRIu64 " and "
                            "controller_host = \'%s\' and controller_pid = %u",
                            datapath_id, controller_host, controller_pid );
  if ( !ret ) {
    return false;
  }

  MYSQL_RES *result = mysql_store_result( db );
  if ( result == NULL ) {
    error( "Failed to retrieve result from database ( %s ).", mysql_error( db ) );
    return false;
  }

  my_ulonglong n_rows = mysql_num_rows( result );
  if ( n_rows == 0 ) {
    debug( "Switch %#" PRIx64 " is not on our duty.", datapath_id );
    mysql_free_result( result );
    return false;
  }

  mysql_free_result( result );

  debug( "Switch %#" PRIx64 " is on our duty.", datapath_id );

  return true;
}


bool
set_switch_port_event_handler( switch_port_event_handler callback, void *user_data ) {
  debug( "Setting a switch port event handler ( callback = %p, user_data = %p ).", callback, user_data );

  port_event_callback = callback;
  port_event_user_data = user_data;

  return true;
}


bool
init_switch( MYSQL *_db ) {
  debug( "Initializing switch/port manager ( _db = %p ).", _db );

  if ( _db == NULL ) {
    error( "MySQL handle must not be null." );
    return false;
  }

  db = _db;

  debug( "Initialization completed successfully ( db = %p ).", db );

  return true;
}


bool
finalize_switch() {
  debug( "Finalizing switch/port manager ( db = %p ).", db );

  if ( db == NULL ) {
    error( "MySQL handle is already unset." );
    return false;
  }

  db = NULL;
  port_event_callback = NULL;
  port_event_user_data = NULL;

  debug( "Finalization completed successfully." );

  return true;
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
