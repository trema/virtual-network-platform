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
#include <getopt.h>
#include <inttypes.h>
#include <limits.h>
#include <linux/limits.h>
#include <mysql/mysql.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include "db.h"
#include "http_client.h"
#include "overlay_network_manager.h"
#include "ovs.h"
#include "slice.h"
#include "switch.h"
#include "transaction_manager.h"
#include "trema.h"
#ifdef UNIT_TEST
#include "bisco_unittest.h"
#endif


static char hostname[ HOST_NAME_MAX + 1 ];
static MYSQL *db = NULL;
static bool initialized = false;
static hash_table *switches_to_disconnect = NULL;


typedef struct {
  int n_transactions;
  bool failed;
} ongoing_transactions;

typedef struct {
  uint8_t table_id;
  uint16_t priority;
  ongoing_transactions *transactions;
  char matches_string[ 256 ];
} flow_entry_to_be_installed;


static void
disconnect_switch_actually( void *user_data ) {
  assert( user_data != NULL );

  uint64_t *datapath_id = user_data;

  assert( switches_to_disconnect != NULL );
  void *deleted = delete_hash_entry( switches_to_disconnect, datapath_id );
  if ( deleted != NULL ) {
    info( "Disconnecting switch ( datapath_id = %#" PRIx64 " ).", *datapath_id );
    disconnect_switch( *datapath_id );
    xfree( deleted );
  }
}


static bool
switch_to_be_disconnected( uint64_t datapath_id ) {
  assert( switches_to_disconnect != NULL );

  uint64_t *entry = lookup_hash_entry( switches_to_disconnect, &datapath_id );
  if ( entry == NULL ) {
    return false;
  }

  return true;
}


static void
schedule_disconnect_switch( uint64_t datapath_id ) {
  assert( switches_to_disconnect != NULL );

  if ( switch_to_be_disconnected( datapath_id ) ) {
    return;
  }

  uint64_t *entry = xmalloc( sizeof( uint64_t ) );
  *entry = datapath_id;

  insert_hash_entry( switches_to_disconnect, entry, entry );

  const time_t delay_sec = ( time_t ) ( rand() % 30 );
  struct itimerspec timer = { { 0, 0 }, { delay_sec, 0 } };
  add_timer_event_callback( &timer, disconnect_switch_actually, entry );

  info( "Switch %#" PRIx64 " is scheduled to be disconnected in %d second%s.",
        datapath_id, ( int ) delay_sec, delay_sec <= 1 ? "" : "s" );
}


static void
unschedule_disconnect_switch( uint64_t datapath_id ) {
  assert( switches_to_disconnect != NULL );

  uint64_t *entry = delete_hash_entry( switches_to_disconnect, &datapath_id );
  if ( entry == NULL ) {
    return;
  }

  delete_timer_event( disconnect_switch_actually, entry );
  xfree( entry );

  info( "Switch %#" PRIx64 " is unscheduled to be disconnected.", datapath_id );
}


static void
default_flow_entry_installed( uint64_t datapath_id, const buffer *message, void *user_data ) {
  UNUSED( message );
  assert( user_data != NULL );

  flow_entry_to_be_installed *entry = user_data;
  assert( entry->transactions != NULL );

  debug( "Default flow entry ( table_id = %#x, priority = %u, match = [%s] ) is installed to switch %#" PRIx64 ".",
         entry->table_id, entry->priority, entry->matches_string, datapath_id );

  entry->transactions->n_transactions--;
  if ( entry->transactions->n_transactions <= 0 && !entry->transactions->failed ) {
    debug( "Sending a features request to switch %#" PRIx64 ".", datapath_id );
    buffer *request = create_features_request( get_transaction_id() );
    bool ret = send_openflow_message( datapath_id, request );
    if ( !ret ) {
      error( "Failed to send a features request to switch %#" PRIx64 ".", datapath_id );
      schedule_disconnect_switch( datapath_id );
    }
    free_buffer( request );
    xfree( entry->transactions );
  }
  xfree( entry );
}


static void
default_flow_entry_not_installed( uint64_t datapath_id, const buffer *message, void *user_data ) {
  assert( message != NULL );
  assert( message->data != NULL );
  assert( message->length > 0 );
  assert( user_data != NULL );

  flow_entry_to_be_installed *entry = user_data;
  assert( entry->transactions != NULL );

  error( "Default flow entry ( table_id = %#x, priority = %u, match = [%s] ) is NOT installed to switch %#" PRIx64 ".",
         entry->table_id, entry->priority, entry->matches_string, datapath_id );
  dump_buffer( message, error );
  entry->transactions->n_transactions--;
  entry->transactions->failed = true;

  xfree( entry );

  schedule_disconnect_switch( datapath_id );
}


static bool
install_default_flow_entry( uint64_t datapath_id, uint8_t table_id, uint16_t priority, const ovs_matches *matches,
                            openflow_actions *actions, ongoing_transactions *transactions ) {
  assert( matches != NULL );
  assert( transactions != NULL );

  flow_entry_to_be_installed *entry = xmalloc( sizeof( flow_entry_to_be_installed ) );
  memset( entry, 0, sizeof( flow_entry_to_be_installed ) );
  ovs_matches_to_string( matches, entry->matches_string, sizeof( entry->matches_string ) );
  entry->table_id = table_id;
  entry->priority = priority;
  transactions->n_transactions++;
  entry->transactions = transactions;

  debug( "Installing a default flow entry ( datapath_id = %#" PRIx64 ", table_id = %#x, priority = %u, matches = [%s] ).",
         datapath_id, entry->table_id, entry->priority, entry->matches_string );

  buffer *flow_mod = create_ovs_flow_mod( get_transaction_id(), 0, OFPFC_MODIFY_STRICT,
                                         entry->table_id, 0, 0, entry->priority, UINT32_MAX,
                                         OFPP_NONE, 0, matches, actions );
  bool ret = execute_transaction( datapath_id, flow_mod,
                                  default_flow_entry_installed, entry,
                                  default_flow_entry_not_installed, entry );
  if ( !ret ) {
    error( "Failed to install a default flow entry ( datapath_id = %#" PRIx64 ", table_id = %#x, "
           "priority = %u, matches = [%s] ).", datapath_id, entry->table_id, entry->priority, entry->matches_string );
    schedule_disconnect_switch( datapath_id );
  }
  free_buffer( flow_mod );

  return ret;
}


static void
ovs_flow_mod_table_id_succeeded( uint64_t datapath_id, const buffer *message, void *user_data ) {
  UNUSED( message );
  UNUSED( user_data );

  debug( "Flow moditication with table id is enabled ( datapath_id = %#" PRIx64 " ).", datapath_id );

  // default flow entries for table #0
  ongoing_transactions *transactions = xmalloc( sizeof( ongoing_transactions ) );
  transactions->n_transactions = 0;
  transactions->failed = false;

  ovs_matches *match = create_ovs_matches();
  bool ret = install_default_flow_entry( datapath_id, 0, 0, match, NULL, transactions );
  delete_ovs_matches( match );
  if ( !ret ) {
    return;
  }
 
  match = create_ovs_matches();
  uint8_t addr[ OFP_ETH_ALEN] = { 0, 0, 0, 0, 0, 0 };
  uint8_t mask[ OFP_ETH_ALEN ] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
  append_ovs_match_eth_dst( match, addr, mask );
  ret = install_default_flow_entry( datapath_id, 0, UINT16_MAX, match, NULL, transactions );
  delete_ovs_matches( match );
  if ( !ret ) {
    return;
  }

  match = create_ovs_matches();
  append_ovs_match_eth_src( match, addr );
  ret = install_default_flow_entry( datapath_id, 0, UINT16_MAX, match, NULL, transactions );
  delete_ovs_matches( match );
  if ( !ret ) {
    return;
  }

  match = create_ovs_matches();
  memset( addr, 0xff, sizeof( addr ) );
  append_ovs_match_eth_src( match, addr );
  ret = install_default_flow_entry( datapath_id, 0, UINT16_MAX, match, NULL, transactions );
  delete_ovs_matches( match );
  if ( !ret ) {
    return;
  }

  // FIXME: hard-coded table id
  // default flow entries for table #2
  match = create_ovs_matches();
  openflow_actions *actions = create_actions();
  append_ovs_action_resubmit_table( actions, OFPP_IN_PORT, 3 );
  install_default_flow_entry( datapath_id, 2, 0, match, actions, transactions );
  delete_actions( actions );
  delete_ovs_matches( match );

  // FIXME: hard-coded table id
  // default flow entries for table #3
  match = create_ovs_matches();
  install_default_flow_entry( datapath_id, 3, 0, match, NULL, transactions );
  delete_ovs_matches( match );
}


static void
ovs_flow_mod_table_id_failed( uint64_t datapath_id, const buffer *message, void *user_data ) {
  UNUSED( message );
  UNUSED( user_data );

  error( "Failed to allow flow modification with table id ( datapath_id = %#" PRIx64 " ).", datapath_id );

  schedule_disconnect_switch( datapath_id );
}


static void
ovs_set_flow_format_succeeded( uint64_t datapath_id, const buffer *message, void *user_data ) {
  UNUSED( message );
  UNUSED( user_data );

  debug( "Flow format is set to Open vSwitch extended mode ( datapath_id = %#" PRIx64 " ).", datapath_id );

  buffer *flow_mod_table_id = create_ovs_flow_mod_table_id( get_transaction_id(), ENABLE_FLOW_MOD_TABLE_ID );
  bool ret = execute_transaction( datapath_id, flow_mod_table_id,
                                  ovs_flow_mod_table_id_succeeded, NULL,
                                  ovs_flow_mod_table_id_failed, NULL );
  if ( !ret ) {
    error( "Failed to allow flow modification with table id ( datapath_id = %#" PRIx64 " ).", datapath_id );
    schedule_disconnect_switch( datapath_id );
  }

  free_buffer( flow_mod_table_id );
}


static void
ovs_set_flow_format_failed( uint64_t datapath_id, const buffer *message, void *user_data ) {
  UNUSED( message );
  UNUSED( user_data );

  error( "Failed to set flow format ( datapath_id = %#" PRIx64 " ).", datapath_id );

  schedule_disconnect_switch( datapath_id );
}


static void
handle_switch_ready( uint64_t datapath_id, void *user_data ) {
  UNUSED( user_data );

  info( "Switch %#" PRIx64 " connected.", datapath_id );

  if ( switch_to_be_disconnected( datapath_id ) ) {
    return;
  }

  buffer *message = create_ovs_set_flow_format( get_transaction_id(), OVSFF_OVSM );
  bool ret = execute_transaction( datapath_id, message,
                                  ovs_set_flow_format_succeeded, NULL,
                                  ovs_set_flow_format_failed, NULL );
  if ( !ret ) {
    error( "Failed to set flow format ( datapath_id = %#" PRIx64 " ).", datapath_id );
    schedule_disconnect_switch( datapath_id );
  }

  free_buffer( message );
}


static void
handle_switch_disconnected( uint64_t datapath_id, void *user_data ) {
  UNUSED( user_data );

  info( "Switch %#" PRIx64 " disconnected.", datapath_id );

  unschedule_disconnect_switch( datapath_id );

  bool ret = delete_switch( datapath_id, hostname, getpid() );
  if ( !ret ) {
    error( "Failed to delete a switch from database ( datapath_id = %#" PRIx64 " ).", datapath_id );
  }
}


static void
handle_features_reply( uint64_t datapath_id, uint32_t transaction_id, uint32_t n_buffers,
                       uint8_t n_tables, uint32_t capabilities, uint32_t actions,
                       const list_element *phy_ports, void *user_data ) {
  debug( "Handling a features reply from switch %#" PRIx64 " ( transaction_id = %#x, n_buffers = %u, n_tables = %u, "
         "capabilities = %#x, actions = %#x, phy_ports = %p, user_data = %p ).",
         datapath_id, transaction_id, n_buffers, n_tables, capabilities, actions, phy_ports, user_data );

  pid_t pid = getpid();
  bool ret = add_switch( datapath_id, hostname, pid );
  if ( !ret ) {
    error( "Failed to add a switch to switch database ( datapath_id = %#" PRIx64
           ", constroller_host = %s, controller_pid = %u ).", datapath_id, hostname, pid );
    return;
  }

  int errors = 0;
  for ( const list_element *e = phy_ports; e != NULL; e = e->next ) {
    struct ofp_phy_port *port = e->data;

    debug( "Adding a port to port database ( port_no = %u, name = %s, hw_addr = %02x:%02x:%02x:%02x:%02x:%02x, "
           "config = %#x, state = %#x, curr = %#x, advertised = %#x, supported = %#x, peer = %#x ).",
           port->port_no, port->name, port->hw_addr[ 0 ], port->hw_addr[ 1 ], port->hw_addr[ 2 ], port->hw_addr[ 3 ], port->hw_addr[ 4 ], port->hw_addr[ 5 ],
           port->config, port->state, port->curr, port->advertised, port->supported, port->peer );

    ret = add_port( datapath_id, port->port_no, port->name );
    if ( !ret ) {
      error( "Failed to add a switch port to port database ( datapath_id = %#" PRIx64 ", "
             "port_no = %u, name = %s ).", datapath_id, port->port_no, port->name );
      errors++;
      break;
    }
  }

  if ( errors != 0 ) {
    ret = delete_switch( datapath_id, hostname, pid );
    if ( !ret ) {
      error( "Failed to delete a switch from switch database ( datapath_id = %#" PRIx64
             ", constroller_host = %s, controller_pid = %u ).", datapath_id, hostname, pid );
    }
    return;
  }

  info( "Switch %#" PRIx64 " is ready to be managed by %s ( pid = %u ).", datapath_id, hostname, pid );
}


static void
handle_port_status( uint64_t datapath_id, uint32_t transaction_id, uint8_t reason,
                    struct ofp_phy_port port, void *user_data ) {
  debug( "Handling port status from switch %#" PRIx64 " ( transaction_id = %#x, reason = %#x, "
         "port = [port_no = %u, name = %s, hw_addr = %02x:%02x:%02x:%02x:%02x:%02x, "
         "config = %#x, state = %#x, curr = %#x, advertised = %#x, supported = %#x, peer = %#x], user_data = %p ).",
         datapath_id, transaction_id, reason, port.port_no, port.name,
         port.hw_addr[ 0 ], port.hw_addr[ 1 ], port.hw_addr[ 2 ], port.hw_addr[ 3 ], port.hw_addr[ 4 ], port.hw_addr[ 5 ],
         port.config, port.state, port.curr, port.advertised, port.supported, port.peer, user_data );

  switch ( reason ) {
    case OFPPR_ADD:
    case OFPPR_MODIFY:
    {
      add_port( datapath_id, port.port_no, port.name );
    }
    break;

    case OFPPR_DELETE:
    {
      delete_port( datapath_id, port.port_no, port.name );
    }
    break;

    default:
    {
      critical( "Undefined port status reason ( %#x ) received from switch %#" PRIx64 ".", reason, datapath_id );
    }
    break;
  }
}


static void
handle_packet_in( uint64_t datapath_id, uint32_t transaction_id, uint32_t buffer_id, uint16_t total_len,
                  uint16_t in_port, uint8_t reason, const buffer *data, void *user_data ) {
  const uint16_t hard_timeout = 60;

  error( "Unexpected Packet-In received from switch %#" PRIx64 ". Discarding all packets for %u seconds. "
         "( transaction_id = %#x, buffer_id = %#x, total_len = %u, in_port = %u, reason = %#x, "
         "data = %p, user_data = %p ).",
         datapath_id, hard_timeout, transaction_id, buffer_id, total_len, in_port, reason, data, user_data );

  struct ofp_match match;
  memset( &match, 0, sizeof( struct ofp_match ) );
  match.wildcards = OFPFW_ALL;
  buffer *message = create_flow_mod( get_transaction_id(), match, 0xffffffffffffffff, OFPFC_MODIFY_STRICT,
                                     0, hard_timeout, UINT16_MAX, UINT32_MAX, OFPP_NONE, 0, NULL );
  bool ret = send_openflow_message( datapath_id, message );
  if ( !ret ) {
    error( "Failed to send a flow modification (modify strict) message." );
  }
  free_buffer( message );
}


static void
handle_flow_removed( uint64_t datapath_id, uint32_t transaction_id, struct ofp_match match,
                     uint64_t cookie, uint16_t priority, uint8_t reason, uint32_t duration_sec,
                     uint32_t duration_nsec, uint16_t idle_timeout, uint64_t packet_count,
                     uint64_t byte_count, void *user_data ) {
  char match_str[ 256 ];
  match_to_string( &match, match_str, sizeof( match_str ) );

  error( "Unexpected flow removed message received ( datapath_id = %#" PRIx64 ", transaction_id = %#x, "
         "match = [%s], cookie = %#" PRIx64 ", priority = %u, reason = %#x, duration = %u.%09u, "
         "idle_timeout = %u, packet_count = %" PRIu64 ", byte_count = %" PRIu64 ", user_data = %p ).",
         datapath_id, transaction_id, match_str, cookie, priority, reason, duration_sec, duration_nsec,
         idle_timeout, packet_count, byte_count, user_data );

  schedule_disconnect_switch( datapath_id );
}


static void
handle_ovs_flow_removed( uint64_t datapath_id, uint32_t transaction_id,
                         uint64_t cookie, uint16_t priority,
#if OVS_VERSION_CODE >= OVS_VERSION( 2, 0, 0 )
                         uint8_t table_id,
#endif
                         uint8_t reason, uint32_t duration_sec,
                         uint32_t duration_nsec, uint16_t idle_timeout, uint64_t packet_count,
                         uint64_t byte_count, const ovs_matches *matches, void *user_data ) {
  char matches_str[ 256 ];
  ovs_matches_to_string( matches, matches_str, sizeof( matches_str ) );

  error( "Unexpected Open vSwitch extended flow removed message received ( datapath_id = %#" PRIx64 ", "
         "transaction_id = %#x, cookie = %#" PRIx64 ", priority = %u, "
#if OVS_VERSION_CODE >= OVS_VERSION( 2, 0, 0 )
         "table_id = %#x, "
#endif
         "reason = %#x, duration = %u.%09u, idle_timeout = %u, packet_count = %" PRIu64
         ", byte_count = %" PRIu64 ", matches = [%s], user_data = %p ).",
         datapath_id, transaction_id, cookie, priority,
#if OVS_VERSION_CODE >= OVS_VERSION( 2, 0, 0 )
         table_id,
#endif
         reason, duration_sec, duration_nsec,
         idle_timeout, packet_count, byte_count, matches_str, user_data );

  schedule_disconnect_switch( datapath_id );
}


static void
handle_list_switches_reply( const list_element *switches, void *user_data ) {
  debug( "Handling a list switches reply ( switches = %p, user_data = %p ).", switches, user_data );

  pid_t pid = getpid();
  for ( const list_element *e = switches; e != NULL; e = e->next ) {
    uint64_t *datapath_id = ( uint64_t * ) e->data;
    debug( "Switch %#" PRIx64 " found.", *datapath_id );
    if ( !switch_on_duty( *datapath_id, hostname, pid ) ) {
      debug( "Switch %#" PRIx64 " is NOT on our duty.", *datapath_id );
      handle_switch_ready( *datapath_id, NULL );
    }
    else {
      debug( "Switch %#" PRIx64 " is already on our duty.", *datapath_id );
    }
  }
}


static void
do_send_list_switches_request() {
  debug( "Sending a list switches request." );

  bool ret = send_list_switches_request( NULL );
  if ( !ret ) {
    error( "Failed to send a list switches request." );
  }
}


void usage() {
  printf(
    "Usage: %s [OPTION]...\n"
    "\n"
    "  -b, --db_host=HOST_NAME         database host\n"
    "  -p, --db_port=PORT              database port\n"
    "  -u, --db_username=USERNAME      database username\n"
    "  -c, --db_password=PASSWORD      database password\n"
    "  -m, --db_name=NAME              database name\n"
    "  -n, --name=SERVICE_NAME         service name\n"
    "  -d, --daemonize                 run in the background\n"
    "  -l, --logging_level=LEVEL       set logging level\n"
    "  -g, --syslog                    output log messages to syslog\n"
    "  -f, --logging_facility=FACILITY set syslog facility\n"
    "  -h, --help                      display this help and exit\n",
    get_executable_name()
  );
}


static void
reset_getopt() {
  optind = 0;
  opterr = 1;
}


static char short_options[] = "b:p:u:c:m:";
static struct option long_options[] = {
  { "db_host", required_argument, NULL, 'b' },
  { "db_port", required_argument, NULL, 'p' },
  { "db_username", required_argument, NULL, 'u' },
  { "db_password", required_argument, NULL, 'c' },
  { "db_name", required_argument, NULL, 'm' },
  { NULL, 0, NULL, 0  },
};


static void
parse_options( int *argc, char **argv[], db_config *config ) {
  debug( "Parsing command arguments ( argc = %d, argv = %p, config = %p ).", *argc, argv, config );

  assert( argc != NULL );
  assert( *argc >= 0 );
  assert( argv != NULL );
  assert( config != NULL );

  int argc_tmp = *argc;
  char *new_argv[ *argc ];

  for ( int i = 0; i <= *argc; ++i ) {
    new_argv[ i ] = ( *argv )[ i ];
  }

  memset( config->host, '\0', sizeof( config->host ) );
  strncpy( config->host, "127.0.0.1", sizeof( config->host ) - 1 );
  config->port = MYSQL_PORT;
  memset( config->username, '\0', sizeof( config->username ) );
  strncpy( config->username, "root", sizeof( config->username ) - 1 );
  memset( config->password, '\0', sizeof( config->password ) );
  strncpy( config->password, "password", sizeof( config->password ) - 1 );
  memset( config->db_name, '\0', sizeof( config->db_name ) );
  strncpy( config->db_name, "vnet", sizeof( config->db_name ) - 1 );

  int c;
  while ( ( c = getopt_long( *argc, *argv, short_options, long_options, NULL ) ) != -1 ) {
    switch ( c ) {
      case 'b':
        if ( optarg != NULL ) {
          debug( "Hostname is specified ( %s ).", optarg );
          memset( config->host, '\0', sizeof( config->host ) );
          strncpy( config->host, optarg, sizeof( config->host ) - 1 );
        }
        else {
          error( "Hostname must be specified." );
          usage();
          exit( EXIT_FAILURE );
        }
        break;

      case 'p':
        if ( optarg != NULL ) {
          debug( "Port number is specified ( %s ).", optarg );
          if ( atoi( optarg ) >= 0 && atoi( optarg ) <= UINT16_MAX ) {
            config->port = ( uint16_t ) atoi( optarg );
          }
          else {
            error( "Invalid port number specified ( %s ).", optarg );
            usage();
            exit( EXIT_FAILURE );
          }
        }
        break;

      case 'u':
        if ( optarg != NULL ) {
          debug( "Username is specified ( %s ).", optarg );
          memset( config->username, '\0', sizeof( config->username ) );
          strncpy( config->username, optarg, sizeof( config->username ) - 1 );
          memset( optarg, '*', strlen( optarg ) );
        }
        else {
          error( "Username must be specified." );
          usage();
          exit( EXIT_FAILURE );
        }
        break;

      case 'c':
        if ( optarg != NULL ) {
          debug( "Password is specified ( %s ).", optarg );
          memset( config->password, '\0', sizeof( config->password ) );
          strncpy( config->password, optarg, sizeof( config->password ) - 1 );
          memset( optarg, '*', strlen( optarg ) );
        }
        else {
          error( "Password must be specified." );
          usage();
          exit( EXIT_FAILURE );
        }
        break;

      case 'm':
        if ( optarg != NULL ) {
          debug( "Database name is specified ( %s ).", optarg );
          memset( config->db_name, '\0', sizeof( config->db_name ) );
          strncpy( config->db_name, optarg, sizeof( config->db_name ) - 1 );
        }
        else {
          error( "Database name must be specified." );
          usage();
          exit( EXIT_FAILURE );
        }
        break;

      default:
        continue;
    }

    if ( optarg == 0 || strchr( new_argv[ optind - 1 ], '=' ) != NULL ) {
      argc_tmp -= 1;
      new_argv[ optind - 1 ] = NULL;
    }
    else {
      argc_tmp -= 2;
      new_argv[ optind - 1 ] = NULL;
      new_argv[ optind - 2 ] = NULL;
    }
  }

  for ( int i = 0, j = 0; i < *argc; ++i ) {
    if ( new_argv[ i ] != NULL ) {
      ( *argv )[ j ] = new_argv[ i ];
      j++;
    }
  }

  *argc = argc_tmp;
  ( *argv )[ *argc ] = NULL;

  reset_getopt();

  debug( "Parsing completed ( host = %s, port = %u, username = %s, password = %s, db_name = %s ).",
         config->host, config->port, config->username, config->password, config->db_name );
}


static void
finalize_vnet_manager() {
  debug( "Finalizing virtual network manager." );

  if ( !initialized ) {
    debug( "Virtual network manager is already finalized or not initialized yet." );
    return;
  }

  assert( db != NULL );

  bool ret = false;
  ret = finalize_transaction_manager();
  if ( !ret ) {
    error( "Failed to finalize transaction manager." );
  }
  ret = finalize_overlay_network_manager();
  if ( !ret ) {
    error( "Failed to finalize overlay network manager." );
  }
  ret = finalize_http_client();
  if ( !ret ) {
    error( "Failed to finalize HTTP client." );
  }
  ret = delete_switch_by_host_pid( hostname, getpid() );
  if ( !ret ) {
    error( "Failed to delete switches from switch/port database." );
  }
  ret = finalize_switch();
  if ( !ret ) {
    error( "Failed to finalize switch/port manager." );
  }
  ret = finalize_slice();
  if ( !ret ) {
    error( "Failed to finalize slice manager." );
  }
  ret = finalize_db( db );
  if ( !ret ) {
    error( "Failed to finalize backend database." );
  }
  db = NULL;

  memset( hostname, '\0', sizeof( hostname ) );

  assert( switches_to_disconnect != NULL );

  hash_entry *e = NULL;
  hash_iterator iter;
  init_hash_iterator( switches_to_disconnect, &iter );
  while ( ( e = iterate_hash_next( &iter ) ) != NULL ) {
    delete_timer_event( disconnect_switch_actually, e->value );
    xfree( e->value );
  }
  delete_hash( switches_to_disconnect );
  switches_to_disconnect = NULL;

  initialized = false;

  debug( "Finalization completed." );
}


static void
do_exit() {
  info( "Terminating virtual network manager ( hostname = %s, pid = %u ).", hostname, getpid() );

  finalize_vnet_manager();
  stop_trema();
}


static void
stop_vnet_manager() {
  // We just set a handler since this function must be signal safe
  set_external_callback( do_exit );
}


static void
set_exit_handler() {
  debug( "Setting up a signal handler." );

  // Override default signal handler so that we can cleanup our world gracefully
  struct sigaction signal_exit;
  memset( &signal_exit, 0, sizeof( struct sigaction ) );

  signal_exit.sa_handler = ( void * ) stop_vnet_manager;
  sigaction( SIGINT, &signal_exit, NULL );
  sigaction( SIGTERM, &signal_exit, NULL );

  debug( "Signal handler is set successfully ( handler = %p ).", signal_exit.sa_handler );
}


static void
do_start() {
  info( "Starting virtual network manager ( hostname = %s, pid = %u ).", hostname, getpid() );

  do_send_list_switches_request();
}


static void
init_vnet_manager( int *argc, char **argv[] ) {
  debug( "Initializing virtual network manager." );

  if ( initialized ) {
    warn( "Virtual network manager is already initialized." );
    return;
  }

  assert( db == NULL );

  memset( hostname, '\0', sizeof( hostname ) );
  int retval = gethostname( hostname, HOST_NAME_MAX );
  hostname[ HOST_NAME_MAX - 1 ] = '\0';
  if ( retval != 0 ) {
    char buf[ 256 ];
    memset( buf, '\0', sizeof( buf ) );
    char *error_string = strerror_r( errno, buf, sizeof( buf ) - 1 );
    die( "Failed to get hostname ( retval = %d, errno = %s [%d] ).", retval, error_string, errno );
  }

  assert( switches_to_disconnect == NULL );
  switches_to_disconnect = create_hash( compare_datapath_id, hash_datapath_id );

  db_config config;
  memset( &config, 0, sizeof( db_config ) );
  parse_options( argc, argv, &config );

  bool ret = false;
  ret = init_db( &db, config );
  if ( !ret ) {
    die( "Failed to initialize backend database." );
  }
  ret = init_slice( db );
  if ( !ret ) {
    die( "Failed to initialize slice manager." );
  }
  ret = init_switch( db );
  if ( !ret ) {
    die( "Failed to initialize switch/port manager." );
  }
  ret = init_transaction_manager();
  if ( !ret ) {
    die( "Failed to initialize transaction manager." );
  }
  ret = init_http_client();
  if ( !ret ) {
    die( "Failed to initialize HTTP client." );
  }
  ret = init_overlay_network_manager( db );
  if ( !ret ) {
    die( "Failed to initialize overlay network manager." );
  }

  set_exit_handler();
  set_list_switches_reply_handler( handle_list_switches_reply );
  set_switch_ready_handler( handle_switch_ready, NULL );
  set_switch_disconnected_handler( handle_switch_disconnected, NULL );
  set_features_reply_handler( handle_features_reply, NULL );
  set_port_status_handler( handle_port_status, NULL );
  set_packet_in_handler( handle_packet_in, NULL );
  set_flow_removed_handler( handle_flow_removed, NULL );
  set_ovs_flow_removed_handler( handle_ovs_flow_removed, NULL );
  set_external_callback( do_start );

  initialized = true;

  debug( "Initialization completed successfully." );
}


int
main( int argc, char *argv[] ) {
  init_trema( &argc, &argv );
  init_vnet_manager( &argc, &argv );

  start_trema();

  finalize_vnet_manager();

  return 0;
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
