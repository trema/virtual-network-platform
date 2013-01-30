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
#include <limits.h>
#include <netinet/ether.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "log.h"
#include "vxlan_ctrl_client.h"


volatile bool running = true;


typedef struct {
  uint8_t type;
  uint32_t vni;
  struct in_addr ip_addr;
  uint16_t port;
  struct ether_addr eth_addr;
  time_t aging_time;
  uint16_t set_bitmap;
} command_options;


static char short_options[] = "asdlfwoebgqn:i:p:m:t:h";

static struct option long_options[] = {
  { "add_instance", no_argument, NULL, 'a' },
  { "set_instance", no_argument, NULL, 's' },
  { "inactivate_instance", no_argument, NULL, 'w' },
  { "activate_instance", no_argument, NULL, 'o' },
  { "del_instance", no_argument, NULL, 'd' },
  { "show_global", no_argument, NULL, 'g' },
  { "list_instances", no_argument, NULL, 'l' },
  { "show_fdb", no_argument, NULL, 'f' },
  { "add_fdb_entry", no_argument, NULL, 'e' },
  { "delete_fdb_entry", no_argument, NULL, 'b' },
  { "quiet", no_argument, NULL, 'q'},
  { "vni", required_argument, NULL, 'n' },
  { "ip", required_argument, NULL, 'i' },
  { "port", required_argument, NULL, 'p' },
  { "mac", required_argument, NULL, 'm' },
  { "aging_time", required_argument, NULL, 't' },
  { "help", no_argument, NULL, 'h' },
  { NULL, 0, NULL, 0  },
};


static void
usage() {
  printf( "Usage: vxlanctl COMMAND [OPTION]...\n"
          "  COMMAND:\n"
          "    -a, --add_instance         Add a VXLAN instance\n"
          "    -s, --set_instance         Set VXLAN parameters\n"
          "    -w, --inactivate_instance  Inactivate a VXLAN instance\n"
          "    -o, --activate_instance    Activate a VXLAN instance\n"
          "    -d, --del_instance         Delete a VXLAN instance\n"
          "    -g, --show_global          Show global settings\n"
          "    -l, --list_instances       List all VXLAN instances\n"
          "    -f, --show_fdb             Show forwarding database\n"
          "    -e, --add_fdb_entry        Add a static forwarding database entry\n"
          "    -b, --delete_fdb_entry     Delete a static forwarding database entry\n"
          "    -h, --help                 Show this help and exit\n"
          "  OPTIONS:\n"
          "    -n, --vni                  Virtual Network Identifier\n"
          "    -i, --ip                   IP address\n"
          "    -p, --port                 UDP port\n"
          "    -m, --mac                  MAC address\n"
          "    -t, --aging_time           Aging time\n"
          "    -q, --quiet                Disable the output of the header.\n"
    );
}


static bool
parse_arguments( int argc, char *argv[], command_options *options ) {
  assert( argv != NULL );
  assert( options != NULL );

  if ( argc <= 1 || argc >= 11 ) {
    return false;
  }

  memset( options, 0, sizeof( command_options ) );
  options->type = MESSAGE_TYPE_MAX;
  options->port = 0;
  options->aging_time = -1;

  bool ret = true;
  int c = -1;
  while ( ( c = getopt_long( argc, argv, short_options, long_options, NULL ) ) != -1 ) {
    switch ( c ) {
      case 'a':
        options->type = ADD_INSTANCE_REQUEST;
        break;

      case 's':
        options->type = SET_INSTANCE_REQUEST;
        break;

      case 'd':
        options->type = DEL_INSTANCE_REQUEST;
        break;

      case 'g':
        if ( options->type != LIST_INSTANCES_REQUEST) {
          options->type = SHOW_GLOBAL_REQUEST;
        }
        options->set_bitmap |= SHOW_GLOBAL;
        break;

      case 'l':
        options->type = LIST_INSTANCES_REQUEST;
        break;

      case 'f':
        options->type = SHOW_FDB_REQUEST;
        break;

      case 'e':
        options->type = ADD_FDB_ENTRY_REQUEST;
        break;

      case 'b':
        options->type = DEL_FDB_ENTRY_REQUEST;
        break;

      case 'w':
        options->type = INACTIVATE_INSTANCE_REQUEST;
        break;

      case 'o':
        options->type = ACTIVATE_INSTANCE_REQUEST;
        break;

      case 'n':
        if ( optarg != NULL ) {
          char *endp = NULL;
          unsigned long int vni = strtoul( optarg, &endp, 0 );
          if ( *endp == '\0' && vni <= 0x00ffffff ) {
            options->vni = ( uint32_t ) vni;
            options->set_bitmap |= SET_VNI;
          }
          else {
            printf( "Invalid VNI value ( %s ).\n", optarg );
            ret &= false;
          }
        }
        else {
          printf( "A VNI must be specified.\n" );
          ret &= false;
        }
        break;

      case 'i':
      {
        if ( optarg != NULL ) {
          int retval = inet_pton( AF_INET, optarg, &options->ip_addr );
          if ( retval != 1 ) {
            printf( "Invalid IP address value ( %s ).\n", optarg );
            ret &= false;
          }
          else {
            options->set_bitmap |= SET_IP_ADDR;
          }
        }
        else {
          printf( "An IP address must be specified.\n" );
          ret &= false;
        }
      }
      break;

      case 'p':
        if ( optarg != NULL ) {
          char *endp = NULL;
          unsigned long int port = strtoul( optarg, &endp, 0 );
          if ( *endp == '\0' && port <= UINT16_MAX ) {
            options->port = ( uint16_t ) port;
            options->set_bitmap |= SET_UDP_PORT;
          }
          else {
            printf( "Invalid UDP port value ( %s ).\n", optarg );
            ret &= false;
          }
        }
        else {
          printf( "A UDP port must be specified.\n" );
          ret &= false;
        }
        break;

      case 'm':
        if ( optarg != NULL ) {
          struct ether_addr *addr = ether_aton_r( optarg, &options->eth_addr );
          if ( addr == NULL ) {
            printf( "Invalid MAC address ( %s ).\n", optarg );
            ret &= false;
          }
          options->set_bitmap |= SET_MAC_ADDR;
        }
        else {
          printf( "A UDP port must be specified.\n" );
          ret &= false;
        }
        break;

      case 't':
        if ( optarg != NULL ) {
          char *endp = NULL;
          long long int aging_time = strtoll( optarg, &endp, 0 );
          if ( *endp == '\0' && aging_time >= 0 && aging_time <= VXLAN_MAX_AGING_TIME ) {
            options->aging_time = ( time_t ) aging_time;
            options->set_bitmap |= SET_AGING_TIME;
          }
          else {
            printf( "Invalid aging time value ( %s ).\n", optarg );
            ret &= false;
          }
        }
        else {
          printf( "An aging time value must be specified.\n" );
          ret &= false;
        }
        break;

      case 'q':
        options->set_bitmap |= DISABLE_HEADER;
        break;

      case 'h':
        usage();
        exit( EXIT_SUCCESS );
        break;

      default:
        ret &= false;
        break;
    }
  }

  switch ( options->type ) {
    case ADD_INSTANCE_REQUEST:
    {
      uint16_t mask = SET_VNI;
      if ( ( options->set_bitmap & mask ) != mask ) {
        ret &= false;
      }
      mask = SET_VNI | SET_IP_ADDR | SET_UDP_PORT | SET_AGING_TIME;
      if ( ( options->set_bitmap & ~mask ) != 0 ) {
        ret &= false;
      }
    }
    break;

    case SET_INSTANCE_REQUEST:
    {
      uint16_t mask = SET_VNI;
      if ( ( options->set_bitmap & mask ) != mask ) {
        ret &= false;
      }
      mask = SET_IP_ADDR | SET_UDP_PORT | SET_AGING_TIME;
      if ( ( options->set_bitmap & mask ) == 0 ) {
        ret &= false;
      }
      mask = SET_VNI | SET_IP_ADDR | SET_UDP_PORT | SET_AGING_TIME;
      if ( ( options->set_bitmap & ~mask ) != 0 ) {
        ret &= false;
      }
    }
    break;

    case INACTIVATE_INSTANCE_REQUEST:
    {
      if ( options->set_bitmap != SET_VNI ) {
        ret &= false;
      }
    }
    break;

    case ACTIVATE_INSTANCE_REQUEST:
    {
      if ( options->set_bitmap != SET_VNI ) {
        ret &= false;
      }
    }
    break;

    case DEL_INSTANCE_REQUEST:
    {
      if ( options->set_bitmap != SET_VNI ) {
        ret &= false;
      }
    }
    break;

    case SHOW_GLOBAL_REQUEST:
    {
      uint16_t mask = SHOW_GLOBAL | DISABLE_HEADER;
      if ( ( options->set_bitmap & ~mask ) != 0 ) {
        ret &= false;
      }
    }
    break;

    case LIST_INSTANCES_REQUEST:
    {
      uint16_t mask = SET_VNI;
      if ( ( options-> set_bitmap & mask ) == 0 )  {
        options->vni = 0xffffffff;
      }
      mask = SET_VNI | SHOW_GLOBAL | DISABLE_HEADER;
      if ( ( options->set_bitmap & ~mask ) != 0 ) {
        ret &= false;
      }
    }
    break;

    case SHOW_FDB_REQUEST:
    {
      if ( options->set_bitmap != SET_VNI ) {
        ret &= false;
      }
    }
    break;

    case ADD_FDB_ENTRY_REQUEST:
    {
      uint16_t mask = SET_VNI | SET_IP_ADDR | SET_MAC_ADDR;
      if ( ( options->set_bitmap & mask ) != mask ) {
        ret &= false;
      }
      mask = SET_VNI | SET_IP_ADDR | SET_MAC_ADDR | SET_AGING_TIME;
      if ( ( options->set_bitmap & ~mask ) != 0 ) {
        ret &= false;
      }
    }
    break;

    case DEL_FDB_ENTRY_REQUEST:
    {
      uint16_t mask = SET_VNI;
      if ( ( options->set_bitmap & mask ) != mask ) {
        ret &= false;
      }
      mask = SET_VNI | SET_MAC_ADDR;
      if ( ( options->set_bitmap & ~mask ) != 0 ) {
        ret &= false;
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
    exit( EXIT_FAILURE );
  }

  init_log( basename( argv[ 0 ] ), LOG_OUTPUT_STDOUT );
  init_vxlan_ctrl_client();

  uint8_t status = SUCCEEDED;
  ret = false;
  switch ( options.type ) {
    case ADD_INSTANCE_REQUEST:
    {
      ret = add_instance( options.vni, options.ip_addr, options.port, options.aging_time, &status );
    }
    break;

    case SET_INSTANCE_REQUEST:
    {
      ret = set_instance( options.vni, options.set_bitmap, options.ip_addr, options.port,
                          options.aging_time, &status );
    }
    break;

    case INACTIVATE_INSTANCE_REQUEST:
    {
      ret = inactivate_instance( options.vni, &status );
    }
    break;

    case ACTIVATE_INSTANCE_REQUEST:
    {
      ret = activate_instance( options.vni, &status );
    }
    break;

    case DEL_INSTANCE_REQUEST:
    {
      ret = delete_instance( options.vni, &status );
    }
    break;

    case SHOW_GLOBAL_REQUEST:
    {
      ret = show_global( options.set_bitmap, &status );
    }
    break;

    case LIST_INSTANCES_REQUEST:
    {
      ret = list_instances( options.vni, options.set_bitmap, &status );
    }
    break;

    case SHOW_FDB_REQUEST:
    {
      ret = show_fdb( options.vni, &status );
    }
    break;

    case ADD_FDB_ENTRY_REQUEST:
    {
      ret = add_fdb_entry( options.vni, options.eth_addr, options.ip_addr, options.aging_time, &status );
    }
    break;

    case DEL_FDB_ENTRY_REQUEST:
    {
      ret = delete_fdb_entry( options.vni, options.eth_addr, &status );
    }
    break;

    default:
    {
      printf( "Undefined command ( %#x ).\n", options.type );
      status = OTHER_ERROR;
      ret = false;
    }
    break;
  }

  if ( !ret ) {
    printf( "Failed to execute a command ( %u ).\n", status );
  }

  finalize_vxlan_ctrl_client();
  finalize_log();

  return status;
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
