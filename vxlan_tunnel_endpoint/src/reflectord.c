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
#include <fcntl.h>
#include <getopt.h>
#include <libgen.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "checks.h"
#include "reflector_ctrl_server.h"
#include "daemon.h"
#include "distributor.h"
#include "ethdev.h"
#include "hash.h"
#include "linked_list.h"
#include "log.h"
#include "queue.h"
#include "receiver.h"
#include "reflector_common.h"


struct {
  char interface[ IFNAMSIZ ];
  uint16_t port;
  uint8_t log_output;
  bool daemonize;
} config;


static const int N_PACKET_BUFFERS = 1024;
static char *program_name = NULL;


static void
create_queues() {
  assert( received_packets == NULL );
  assert( free_packet_buffers == NULL );

  received_packets = create_queue();
  assert( received_packets != NULL );
  free_packet_buffers = create_queue();
  assert( free_packet_buffers != NULL );

  for ( int i = 0; i < N_PACKET_BUFFERS; i++ ) {
    packet_buffer *buffer = malloc( sizeof( packet_buffer ) );
    enqueue( free_packet_buffers, buffer );
  }
}


static void
delete_queues() {
  assert( received_packets != NULL );
  assert( free_packet_buffers != NULL );

  delete_queue( received_packets );
  delete_queue( free_packet_buffers );
}


static void
stop( int signum ) {
  UNUSED( signum );

  running = false;
}


static void
set_signal_handler() {
  struct sigaction signal_exit;
  memset( &signal_exit, 0, sizeof( struct sigaction ) );

  signal_exit.sa_handler = stop;
  sigaction( SIGINT, &signal_exit, NULL );
  sigaction( SIGTERM, &signal_exit, NULL );

  struct sigaction signal_ignore;
  memset( &signal_ignore, 0, sizeof( struct sigaction ) );

  signal_exit.sa_handler = SIG_IGN;
  sigaction( SIGPIPE, &signal_ignore, NULL );
}


static char short_options[] = "i:p:sdh";

static struct option long_options[] = {
  { "interface", required_argument, NULL, 'i' },
  { "port", required_argument, NULL, 'p' },
  { "syslog", no_argument, NULL, 's' },
  { "daemonize", no_argument, NULL, 'd' },
  { "help", no_argument, NULL, 'h' },
  { NULL, 0, NULL, 0  },
};


static void
usage() {
  printf( "Usage: reflectord -i INTERFACE [OPTION]...\n"
          "  OPTIONS:\n"
          "    -p, --port       UDP port for receiving VXLAN packets\n"
          "    -s, --syslog     Output log messages to syslog\n"
          "    -d, --daemonize  Daemonize\n"
          "    -h, --help       Display this help and exit\n"
    );
}


static bool
parse_arguments( int argc, char *argv[] ) {
  assert( argv != NULL );

  if ( argc <= 2 ) {
    return false;
  }

  memset( config.interface, '\0', sizeof( config.interface ) );
  config.log_output = LOG_OUTPUT_STDOUT;
  config.daemonize = false;
  config.port = VXLAN_DEFAULT_UDP_PORT;

  bool ret = true;
  int c;
  while ( ( c = getopt_long( argc, argv, short_options, long_options, NULL ) ) != -1 ) {
    switch ( c ) {
      case 'i':
        if ( optarg != NULL ) {
          strncpy( config.interface, optarg, sizeof( config.interface ) - 1 );
        }
        else {
          ret &= false;
        }
        break;

      case 'p':
        if ( optarg != NULL ) {
          char *endp = NULL;
          unsigned long port = strtoul( optarg, &endp, 0 );
          if ( *endp != '\0' || ( *endp == '\0' && port > UINT16_MAX ) ) {
            printf( "Invalid port number ( %s ).\n", optarg );
            ret &= false;
          }
          else {
            config.port = ( uint16_t ) port;
          }
        }
        else {
          ret &= false;
        }
        break;

      case 's':
        config.log_output = LOG_OUTPUT_SYSLOG;
        break;

      case 'd':
        config.daemonize = true;
        break;

      case 'h':
        usage();
        exit( SUCCEEDED );
        break;

      default:
        ret &= false;
        break;
    }
  }

  if ( config.daemonize ) {
    config.log_output &= ( uint8_t ) ~LOG_OUTPUT_STDOUT;
  }

  if ( strlen( config.interface ) == 0 ) {
    ret &= false;
  }

  return ret;
}


static void
start_reflector() {
  while ( running ) {
    bool ret = run_reflector_ctrl_server();
    if ( !ret ) {
      running = false;
    }
  }
}


static bool
init_reflector( const char *name, ethdev **dev ) {
  assert( dev != NULL );

  program_name = strdup( name );

  if ( config.daemonize ) {
    daemonize();
  }

  init_log( program_name, config.log_output );

  info( "Starting Jumper Wire - VXLAN packet reflector daemon ( pid = %u ).", getpid() );

  bool ret = create_pid_file( program_name );
  if ( !ret ) {
    error( "Failed to create a pid file." );
    return false;
  }

  set_signal_handler();

  *dev = NULL;
  ret = init_ethdev( config.interface, config.port, dev );
  if ( !ret ) {
    error( "Failed to initialize an Ethernet interface ( %s ).", config.interface );
    return false;
  }

  create_queues();

  pthread_attr_t attr;
  pthread_attr_init( &attr );
  int retval = pthread_create( &distributor_thread, &attr, distributor_main, *dev );
  if ( retval != 0 ) {
    critical( "Failed to create a distributor thread ( ret = %d ).", ret );
    close_ethdev( *dev );
    delete_queues();
    return false;
  }

  receiver_options *options = malloc( sizeof( receiver_options ) );
  memset( options, 0, sizeof( receiver_options ) );
  options->dev = *dev;
  options->port = config.port;
  retval = pthread_create( &receiver_thread, &attr, receiver_main, options );
  if ( retval != 0 ) {
    critical( "Failed to create a receiver thread ( ret = %d ).", ret );
    close_ethdev( *dev );
    delete_queues();
    return false;
  }

  ret = init_reflector_ctrl_server( *dev );
  if ( !ret ) {
    critical( "Failed to initialize control interface." );
    pthread_cancel( distributor_thread );
    pthread_cancel( receiver_thread );
    close_ethdev( *dev );
    delete_queues();
    return false;
  }

  return true;
}


static bool
finalize_reflector( ethdev *dev ) {
  assert( dev != NULL );

  info( "Terminating Jumper Wire - VXLAN packet reflector daemon ( pid = %u ).", getpid() );

  bool ret = finalize_reflector_ctrl_server();
  if ( !ret ) {
    error( "Failed to finalize control interface." );
    return false;
  }

  void *retval = NULL;
  while ( pthread_tryjoin_np( distributor_thread, &retval ) == EBUSY ||
          pthread_tryjoin_np( receiver_thread, &retval ) == EBUSY ) {
    struct timespec req = { 0, 50000000 };
    nanosleep( &req, NULL );
  }

  ret = close_ethdev( dev );
  if ( !ret ) {
    error( "Failed to close an Ethernet interface ( %s ).", config.interface );
    return false;
  }

  delete_queues();

  finalize_log();

  ret = remove_pid_file( program_name );
  if ( !ret ) {
    error( "Failed to remove a pid file." );
    return false;
  }

  if ( program_name != NULL ) {
    free( program_name );
  }

  return true;
}


int
main( int argc, char *argv[] ) {
  int status = SUCCEEDED;

  if ( geteuid() != 0 ) {
    printf( "%s must be run with root privilege.\n", argv[ 0 ] );
    exit( OTHER_ERROR );
  }

  bool ret = parse_arguments( argc, argv );
  if ( !ret ) {
    usage();
    exit( INVALID_ARGUMENT );
  }

  if ( pid_file_exists( basename( argv[ 0 ] ) ) ) {
    printf( "Another %s is running.\n", basename( argv[ 0 ] ) );
    exit( ALREADY_RUNNING );
  }

  ethdev *dev = NULL;
  ret = init_reflector( basename( argv[ 0 ] ), &dev );
  if ( !ret ) {
    status = OTHER_ERROR;
    goto error;
  }

  start_reflector();

  ret = finalize_reflector( dev );
  if ( !ret ) {
    status = OTHER_ERROR;
    goto error;
  }

  return SUCCEEDED;

error:
  remove_pid_file( basename( argv[ 0 ] ) );
  exit( status );
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
