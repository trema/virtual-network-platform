/*
 * Author: Yasunobu Chiba
 *
 * Copyright (C) 2012 NEC Corporation
 * NEC Confidential
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
}


static void
install_default_flow_entry( uint64_t datapath_id, uint8_t table_id, uint16_t priority, const ovs_matches *matches,
                            ongoing_transactions *transactions ) {
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

  buffer *flow_mod = create_ovs_flow_mod( get_transaction_id(), get_cookie(), OFPFC_MODIFY_STRICT,
                                         entry->table_id, 0, 0, entry->priority, UINT32_MAX,
                                         OFPP_NONE, 0, matches, NULL );
  bool ret = execute_transaction( datapath_id, flow_mod,
                                  default_flow_entry_installed, entry,
                                  default_flow_entry_not_installed, entry );
  if ( !ret ) {
    error( "Failed to install a default flow entry ( datapath_id = %#" PRIx64 ", table_id = %#x, "
           "priority = %u, matches = [%s] ).", datapath_id, entry->table_id, entry->priority, entry->matches_string );
  }
  free_buffer( flow_mod );
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
  install_default_flow_entry( datapath_id, 0, 0, match, transactions );
  delete_ovs_matches( match );
 
  match = create_ovs_matches();
  uint8_t addr[ OFP_ETH_ALEN] = { 0, 0, 0, 0, 0, 0 };
  uint8_t mask[ OFP_ETH_ALEN ] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
  append_ovs_match_eth_dst( match, addr, mask );
  install_default_flow_entry( datapath_id, 0, UINT16_MAX, match, transactions );
  delete_ovs_matches( match );

  match = create_ovs_matches();
  append_ovs_match_eth_src( match, addr );
  install_default_flow_entry( datapath_id, 0, UINT16_MAX, match, transactions );
  delete_ovs_matches( match );

  match = create_ovs_matches();
  memset( addr, 0xff, sizeof( addr ) );
  append_ovs_match_eth_src( match, addr );
  install_default_flow_entry( datapath_id, 0, UINT16_MAX, match, transactions );
  delete_ovs_matches( match );

  // default flow entries for table #2
  match = create_ovs_matches();
  install_default_flow_entry( datapath_id, 2, 0, match, transactions );
  delete_ovs_matches( match );
}


static void
ovs_flow_mod_table_id_failed( uint64_t datapath_id, const buffer *message, void *user_data ) {
  UNUSED( message );
  UNUSED( user_data );

  error( "Failed to allow flow modification with table id ( datapath_id = %#" PRIx64 " ).", datapath_id );
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
  }

  free_buffer( flow_mod_table_id );
}


static void
ovs_set_flow_format_failed( uint64_t datapath_id, const buffer *message, void *user_data ) {
  UNUSED( message );
  UNUSED( user_data );

  error( "Failed to set flow format ( datapath_id = %#" PRIx64 " ).", datapath_id );
}


static void
handle_switch_ready( uint64_t datapath_id, void *user_data ) {
  UNUSED( user_data );

  info( "Switch %#" PRIx64 " connected.", datapath_id );

  buffer *message = create_ovs_set_flow_format( get_transaction_id(), OVSFF_OVSM );
  bool ret = execute_transaction( datapath_id, message,
                                  ovs_set_flow_format_succeeded, NULL,
                                  ovs_set_flow_format_failed, NULL );
  if ( !ret ) {
    error( "Failed to set flow format ( datapath_id = %#" PRIx64 " ).", datapath_id );
  }

  free_buffer( message );
}


static void
handle_switch_disconnected( uint64_t datapath_id, void *user_data ) {
  UNUSED( user_data );

  info( "Switch %#" PRIx64 " disconnected.", datapath_id );

  bool ret = delete_switch( datapath_id );
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
    ret = delete_switch( datapath_id );
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
    "  -b, --db_host=HOST_NAME     database host\n"
    "  -p, --db_port=PORT          database port\n"
    "  -u, --db_username=USERNAME  database username\n"
    "  -c, --db_password=PASSWORD  database password\n"
    "  -m, --db_name=NAME          database name\n"
    "  -n, --name=SERVICE_NAME     service name\n"
    "  -d, --daemonize             run in the background\n"
    "  -l, --logging_level=LEVEL   set logging level\n"
    "  -g, --syslog                output log messages to syslog\n"
    "  -h, --help                  display this help and exit\n",
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
