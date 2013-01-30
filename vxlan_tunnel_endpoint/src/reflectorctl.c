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


#include <arpa/inet.h>
#include <assert.h>
#include <getopt.h>
#include <libgen.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include "reflector_ctrl_client.h"
#include "log.h"


volatile bool running = true;


typedef struct {
  uint8_t type;
  uint32_t vni;
  struct in_addr ip_addr;
  uint16_t port;
  uint16_t set_bitmap;
} command_options;


static char short_options[] = "asdln:i:p:h";

static struct option long_options[] = {
  { "add_tep", no_argument, NULL, 'a' },
  { "set_tep", no_argument, NULL, 's' },
  { "del_tep", no_argument, NULL, 'd' },
  { "list_tep", no_argument, NULL, 'l' },
  { "vni", required_argument, NULL, 'n' },
  { "ip", required_argument, NULL, 'i' },
  { "port", required_argument, NULL, 'p' },
  { "help", no_argument, NULL, 'h' },
  { NULL, 0, NULL, 0  },
};


static void
usage() {
  printf( "Usage: reflectorctl COMMAND [OPTION]...\n"
          "  COMMAND:\n"
          "    -a, --add_tep       Add a tunnel endpoint\n"
          "    -d, --del_tep       Delete a tunnel endpoint\n"
          "    -s, --set_tep       Set tunnel endpoint parameters\n"
          "    -l, --list_tep      List tunnel endpoints\n"
          "    -h, --help          Show this help and exit\n"
          "  OPTIONS:\n"
          "    -n, --vni           Virtual Network Identifier\n"
          "    -i, --ip            IP address\n"
          "    -p, --port          Destination UDP port\n"
    );
}


static bool
parse_arguments( int argc, char *argv[], command_options *options ) {
  assert( argv != NULL );
  assert( options != NULL );

  if ( argc <= 1 || argc >= 11 ) {
    return false;
  }

  bool ret = true;

  memset( options, 0, sizeof( command_options ) );
  options->type = MESSAGE_TYPE_MAX;
  options->port = 0;

  int c;
  while ( ( c = getopt_long( argc, argv, short_options, long_options, NULL ) ) != -1 ) {
    switch ( c ) {
      case 'a':
        options->type = ADD_TEP_REQUEST;
        break;

      case 's':
        options->type = SET_TEP_REQUEST;
        break;

      case 'd':
        options->type = DEL_TEP_REQUEST;
        break;

      case 'l':
        options->type = LIST_TEP_REQUEST;
        break;

      case 'n':
        if ( optarg != NULL ) {
          char *endp = NULL;
          unsigned long int vni = strtoul( optarg, &endp, 0 );
          if ( *endp == '\0' && vni <= 0x00ffffff ) {
            options->vni = ( uint32_t ) vni;
            options->set_bitmap |= SET_TEP_VNI;
          }
          else {
            printf( "Invalid VNI value ( %s ).\n", optarg );
            ret &= false;
          }
        }
        else {
          ret &= false;
        }
        break;

      case 'i':
        if ( optarg != NULL ) {
          int retval = inet_aton( optarg, &options->ip_addr );
          if ( retval != 0 ) {
            options->set_bitmap |= SET_TEP_IP_ADDR;
          }
          else {
            printf( "Invalid IP address ( %s ).\n", optarg );
            ret &= false;
          }
        }
        else {
          ret &= false;
        }
        break;

      case 'p':
        if ( optarg != NULL ) {
          char *endp = NULL;
          unsigned long int port = strtoul( optarg, &endp, 0 );
          if ( *endp == '\0' && port <= UINT16_MAX ) {
            options->port = ( uint16_t ) port;
            options->set_bitmap |= SET_TEP_PORT;
          }
          else {
            printf( "Invalid UDP port value ( %s ).\n", optarg );
            ret &= false;
          }
        }
        else {
          ret &= false;
        }
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

  switch ( options->type ) {
    case ADD_TEP_REQUEST:
    {
      uint16_t mask = SET_TEP_VNI | SET_TEP_IP_ADDR;
      if ( ( options->set_bitmap & mask ) != mask ) {
        ret &= false;
      }
    }
    break;

    case SET_TEP_REQUEST:
    {
      uint16_t mask = SET_TEP_VNI | SET_TEP_IP_ADDR;
      if ( ( options->set_bitmap & mask ) != mask ) {
        ret &= false;
      }
      mask = SET_TEP_PORT;
      if ( ( options->set_bitmap & mask ) == 0 ) {
        ret &= false;
      }
    }
    break;

    case DEL_TEP_REQUEST:
    {
      uint16_t mask = SET_TEP_VNI | SET_TEP_IP_ADDR;
      if ( ( options->set_bitmap & mask ) != mask ) {
        ret &= false;
      }
    }
    break;

    case LIST_TEP_REQUEST:
    {
      uint16_t mask = SET_TEP_VNI;
      if ( ( options->set_bitmap & mask ) != mask ) {
        options->vni = VNI_ANY;
      }
    }
    break;

    default:
    {
      ret &= false;
    }
    break;
  }

  return ret;
}


int
main( int argc, char *argv[] ) {
  command_options options;
  bool ret = parse_arguments( argc, argv, &options );
  if ( !ret ) {
    usage();
    exit( INVALID_ARGUMENT );
  }

  init_log( basename( argv[ 0 ] ), LOG_OUTPUT_STDOUT );
  init_reflector_ctrl_client();

  uint8_t status = SUCCEEDED;
  ret = false;
  switch ( options.type ) {
    case ADD_TEP_REQUEST:
    {
      ret = add_tep( options.vni, options.ip_addr, options.port, &status );
    }
    break;

    case SET_TEP_REQUEST:
    {
      ret = set_tep( options.vni, options.ip_addr, options.set_bitmap, options.port, &status );
    }
    break;

    case DEL_TEP_REQUEST:
    {
      ret = delete_tep( options.vni, options.ip_addr, &status );
    }
    break;

    case LIST_TEP_REQUEST:
    {
      ret = list_tep( options.vni, &status );
    }
    break;

    default:
    {
      printf( "Undefined command ( %#x ).\n", options.type );
      status = OTHER_ERROR;
    }
    break;
  }

  if ( !ret ) {
    printf( "Failed to execute a command ( %u ).\n", status );
  }

  finalize_reflector_ctrl_client();
  finalize_log();

  return status;
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
