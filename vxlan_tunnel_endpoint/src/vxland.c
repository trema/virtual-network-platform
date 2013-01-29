/*
 * Copryright (C) 2012 upa@haeena.net
 * Copryright (C) 2012-2013 NEC Corporation
 */


#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <getopt.h>
#include <limits.h>
#include <net/if.h>
#include <net/if.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/timerfd.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>
#include "checks.h"
#include "daemon.h"
#include "fdb.h"
#include "iftap.h"
#include "log.h"
#include "net.h"
#include "vxlan_common.h"
#include "vxlan_ctrl_server.h"
#include "vxlan_instance.h"
#include "wrapper.h"


volatile bool running = true;
static struct vxlan vxlan;
static char *program_name = NULL;


static void
process_vxlan( void ) {
  char buf[ VXLAN_PACKET_BUF_LEN ];
  memset( buf, 0, sizeof( buf ) );

  vxlan.timerfd = timerfd_create( CLOCK_MONOTONIC, TFD_NONBLOCK );
  if ( vxlan.timerfd < 0 ) {
    error( "Failed to create timerfd." );
    exit( OTHER_ERROR );
  }
  struct itimerspec timer;
  memset( &timer, 0, sizeof( struct itimerspec ) );
  timer.it_value.tv_sec = 5;
  timer.it_interval.tv_sec = 5;
  timerfd_settime( vxlan.timerfd, 0, &timer, 0 );

  int fd_max = -1;
  if ( vxlan.udp_sock > vxlan.timerfd ) {
    fd_max = vxlan.udp_sock;
  }
  else {
    fd_max = vxlan.timerfd;
  }

  // From Internet
  while ( running ) {
    fd_set fds;
    FD_ZERO( &fds );
    FD_SET( vxlan.udp_sock, &fds );
    FD_SET( vxlan.timerfd, &fds );

    struct timespec timeout = { 1, 0 };
    int ret = pselect( fd_max + 1, &fds, NULL, NULL, &timeout, NULL );
    if ( ret < 0 ) {
      if ( errno == EINTR ) {
        continue;
      }
      // FIXME: handle error properly
      break;
    }
    else if ( ret == 0 ) {
      continue;
    }

    if ( FD_ISSET( vxlan.timerfd, &fds ) ) {
      uint64_t timer_count = 0;
      read( vxlan.timerfd, &timer_count, sizeof( timer_count ) );
      update_interface_state();
    }

    if ( !FD_ISSET( vxlan.udp_sock, &fds ) ) {
      continue;
    }

    ssize_t len = 0;
    struct sockaddr_in addr;
    socklen_t socklen = sizeof( addr );
    memset( &addr, 0, sizeof( addr ) );
    if ( ( len = recvfrom( vxlan.udp_sock, buf, sizeof( buf ), 0,
                           ( struct sockaddr * ) &addr, &socklen ) ) < 0 ) {
      continue;
    }

    if ( !vxlan.active ) {
      continue;
    }

    struct vxlanhdr *vhdr = ( struct vxlanhdr * ) buf;
    struct vxlan_instance *instance = NULL;
    if ( ( instance = search_hash( &vxlan.instances, vhdr->vni ) ) == NULL ) {
      continue;
    }

    if ( !instance->activated ) {
      continue;
    }

    struct ether_header *ether = ( struct ether_header * ) ( buf + sizeof( struct vxlanhdr ) );
    process_fdb_etherframe_from_vxlan( instance, ether, &addr );
    send_etherframe_from_vxlan_to_local( instance, ether, ( size_t ) len - sizeof( struct vxlanhdr ) );
  }

  close( vxlan.timerfd );
}


static void
stop( int signal ) {
  UNUSED( signal );

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


static char short_options[] = "shm:di:p:a:f:t:";

static struct option long_options[] = {
  { "syslog", no_argument, NULL, 's' },
  { "help", no_argument, NULL, 'h' },
  { "daemonize", no_argument, NULL, 'd' },
  { "interface", required_argument, NULL, 'i' },
  { "port", required_argument, NULL, 'p' },
  { "flooding_address", required_argument, NULL, 'a' },
  { "flooding_port", required_argument, NULL, 'f' },
  { "aging_time", required_argument, NULL, 't' },
  { NULL, 0, NULL, 0  },
};


static void
usage() {
  printf( "Usage: vxland -i INTERFACE [OPTION]...\n"
          "  -i, --interface         Network interface\n"
          "  -p, --port              UDP port for receiving VXLAN packets\n"
          "  -a, --flooding_address  Default destination IP address for sending flooding packets\n"
          "  -f, --flooding_port     Default destination UDP port for sending flooding packets\n"
          "  -t, --aging_time        Default aging time\n"
          "  -s, --syslog            Output log messages to syslog\n"
          "  -d, --daemonize         Daemonize\n"
          "  -h, --help              Show this help and exit.\n" );
}


static bool
parse_arguments( int argc, char *argv[] ) {
  assert( argv != NULL );

  if ( argc <= 2 ) {
    return false;
  }

  memset( &vxlan, 0, sizeof( vxlan ) );
  init_hash( &vxlan.instances, VXLAN_VNISIZE );
  vxlan.log_output = LOG_OUTPUT_STDOUT;
  vxlan.port = VXLAN_DEFAULT_UDP_PORT;
  vxlan.daemonize = false; 
  memset( vxlan.ifname, '\0', sizeof( vxlan.ifname ) );
  inet_pton( AF_INET, VXLAN_DEFAULT_FLOODING_ADDR, &vxlan.flooding_addr );
  vxlan.flooding_port = vxlan.port;
  vxlan.aging_time = VXLAN_DEFAULT_AGING_TIME;

  bool flooding_port_specified = false;

  int ret = true;
  int c = -1;
  while ( ( c = getopt_long( argc, argv, short_options, long_options, NULL ) ) != -1 ) {
    switch ( c ) {
      case 's':
      {
        vxlan.log_output |= LOG_OUTPUT_SYSLOG;
      }
      break;

      case 'i':
      {
        if ( optarg != NULL ) {
          strncpy( vxlan.ifname, optarg, sizeof( vxlan.ifname ) - 1 );
        }
        else {
          ret &= false;
        }
      }
      break;

      case 'p':
      {
        if ( optarg != NULL ) {
          char *endp = NULL;
          unsigned long port = strtoul( optarg, &endp, 0 );
          if ( *endp != '\0' || ( *endp == '\0' && port > UINT16_MAX ) ) {
            printf( "Invalid port number ( %s ).\n", optarg );
            ret &= false;
          }
          else {
            vxlan.port = ( uint16_t ) port;
          }
        }
        else {
          ret &= false;
        }
      }
      break;

      case 'a':
      {
        if ( optarg != NULL ) {
          int ret = inet_pton( AF_INET, optarg, &vxlan.flooding_addr );
          if ( ret != 1 ) {
            printf( "Invalid IP address ( %s ).\n", optarg );
            ret &= false;
          }
        }
        else {
          ret &= false;
        }
      }
      break;

      case 'f':
      {
        if ( optarg != NULL ) {
          char *endp = NULL;
          unsigned long port = strtoul( optarg, &endp, 0 );
          if ( *endp != '\0' || ( *endp == '\0' && port > UINT16_MAX ) ) {
            printf( "Invalid port number ( %s ).\n", optarg );
            ret &= false;
          }
          else {
            vxlan.flooding_port = ( uint16_t ) port;
            flooding_port_specified = true;
          }
        }
        else {
          ret &= false;
        }
      }
      break;

      case 't':
      {
        if ( optarg != NULL ) {
          char *endp = NULL;
          unsigned long aging_time = strtoul( optarg, &endp, 0 );
          if ( *endp != '\0' && aging_time > VXLAN_MAX_AGING_TIME ) {
            printf( "Invalid aging time value ( %s ).\n", optarg );
            ret &= false;
          }
          else {
            vxlan.aging_time = ( time_t ) aging_time;
          }
        }
        else {
          ret &= false;
        }
      }
      break;

      case 'd':
      {
        vxlan.daemonize = true;
      }
      break;

      case 'h':
      {
        usage();
        exit( SUCCEEDED );
      }
      break;

      default:
      {
        ret &= false;
      }
      break;
    }
  }

  if ( vxlan.daemonize ) {
    vxlan.log_output &= ( uint8_t ) ~LOG_OUTPUT_STDOUT;
  }

  if ( !flooding_port_specified ) {
    vxlan.flooding_port = vxlan.port;
  }

  if ( strlen( vxlan.ifname ) == 0 ) {
    ret &= false;
  }

  return ret;
}


static bool
init_vxland( const char *name ) {
  assert( name != NULL );

  program_name = strdup( name );

  if ( vxlan.daemonize ) {
    finalize_log();
    daemonize();
    init_log( name, vxlan.log_output );
  }

  bool ret = create_pid_file( program_name );
  if ( !ret ) {
    return false;
  }

  set_signal_handler();

  ret = init_net( &vxlan );
  if ( !ret ) {
    return false;
  }

  ret = init_vxlan_instances( &vxlan );
  if ( !ret ) {
    return false;
  }

  ret = init_vxlan_ctrl_server( &vxlan );
  if ( !ret ) {
    return false;
  }

  return true;
}


static bool
finalize_vxland() {
  bool ret = true;

  ret &= finalize_vxlan_ctrl_server();
  ret &= finalize_vxlan_instances();
  ret &= finalize_net();

  if ( program_name != NULL ) {
    ret &= remove_pid_file( program_name );
    free( program_name );
  }

  return ret;
}


int
main( int argc, char *argv[] ) {
  int status = SUCCEEDED;

  if ( geteuid() != 0 ) {
    printf( "%s must be run with root privilege.\n", basename( argv[ 0 ] ) );
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

  init_log( basename( argv[ 0 ] ), vxlan.log_output );

  info( "Starting Jumper Wire - VXLAN daemon ( pid = %u ).", getpid() );

  ret = init_vxland( basename( argv[ 0 ] ) );
  if ( !ret ) {
    error( "Failed to initialize Jumper Wire - VXLAN daemon ( pid = %u ).", getpid() );
    status = OTHER_ERROR;
    goto error;
  }

  start_vxlan_ctrl_server();
  process_vxlan();

  info( "Terminating Jumper Wire - VXLAN daemon ( pid = %u ).", getpid() );

  finalize_vxland();
  finalize_log();

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
