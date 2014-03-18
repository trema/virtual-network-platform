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
#include <inttypes.h>
#include <netinet/in.h>
#include <strings.h>
#include "ovs_byteorder.h"
#include "ovs_match.h"
#include "trema.h"


static void
hton_ovs_match_header( ovs_match_header *dst, const ovs_match_header *src ) {
  assert( dst != NULL );
  assert( src != NULL );

  *dst = htonl( *src );
}


static void
hton_ovs_match_8( ovs_match_header *dst, const ovs_match_header *src ) {
  assert( dst != NULL );
  assert( src != NULL );

  hton_ovs_match_header( dst, src );

  const uint8_t *h = ( const uint8_t * ) ( ( const char * ) src + sizeof( ovs_match_header ) );
  uint8_t *n = ( uint8_t * ) ( ( char * ) dst + sizeof( ovs_match_header ) );
  *n = *h;
}


static void
hton_ovs_match_8w( ovs_match_header *dst, const ovs_match_header *src ) {
  assert( dst != NULL );
  assert( src != NULL );

  hton_ovs_match_header( dst, src );

  const uint8_t *value_h = ( const uint8_t * ) ( ( const char * ) src + sizeof( ovs_match_header ) );
  uint8_t *value_n = ( uint8_t * ) ( ( char * ) dst + sizeof( ovs_match_header ) );
  *value_n = *value_h;

  const uint8_t *mask_h = ( const uint8_t * ) ( ( const char * ) value_h + sizeof( uint8_t ) );
  uint8_t *mask_n = ( uint8_t * ) ( ( char * ) value_n + sizeof( uint8_t ) );
  *mask_n = *mask_h;
}


static void
hton_ovs_match_16( ovs_match_header *dst, const ovs_match_header *src ) {
  assert( dst != NULL );
  assert( src != NULL );

  hton_ovs_match_header( dst, src );

  const uint16_t *h = ( const uint16_t * ) ( ( const char * ) src + sizeof( ovs_match_header ) );
  uint16_t *n = ( uint16_t * ) ( ( char * ) dst + sizeof( ovs_match_header ) );
  *n = htons( *h );
}


static void
hton_ovs_match_16w( ovs_match_header *dst, const ovs_match_header *src ) {
  assert( dst != NULL );
  assert( src != NULL );

  hton_ovs_match_header( dst, src );

  const uint16_t *value_h = ( const uint16_t * ) ( ( const char * ) src + sizeof( ovs_match_header ) );
  uint16_t *value_n = ( uint16_t * ) ( ( char * ) dst + sizeof( ovs_match_header ) );
  *value_n = htons( *value_h );

  const uint16_t *mask_h = ( const uint16_t * ) ( ( const char * ) value_h + sizeof( uint16_t ) );
  uint16_t *mask_n = ( uint16_t * ) ( ( char * ) value_n + sizeof( uint16_t ) );
  *mask_n = htons( *mask_h );
}


static void
hton_ovs_match_32( ovs_match_header *dst, const ovs_match_header *src ) {
  assert( dst != NULL );
  assert( src != NULL );

  hton_ovs_match_header( dst, src );

  const uint32_t *h = ( const uint32_t * ) ( ( const char * ) src + sizeof( ovs_match_header ) );
  uint32_t *n = ( uint32_t * ) ( ( char * ) dst + sizeof( ovs_match_header ) );
  *n = htonl( *h );
}


static void
hton_ovs_match_32w( ovs_match_header *dst, const ovs_match_header *src ) {
  assert( dst != NULL );
  assert( src != NULL );

  hton_ovs_match_header( dst, src );

  const uint32_t *value_h = ( const uint32_t * ) ( ( const char * ) src + sizeof( ovs_match_header ) );
  uint32_t *value_n = ( uint32_t * ) ( ( char * ) dst + sizeof( ovs_match_header ) );
  *value_n = htonl( *value_h );

  const uint32_t *mask_h = ( const uint32_t * ) ( ( const char * ) value_h + sizeof( uint32_t ) );
  uint32_t *mask_n = ( uint32_t * ) ( ( char * ) value_n + sizeof( uint32_t ) );
  *mask_n = htonl( *mask_h );
}


static void
hton_ovs_match_64( ovs_match_header *dst, const ovs_match_header *src ) {
  assert( dst != NULL );
  assert( src != NULL );

  hton_ovs_match_header( dst, src );

  const uint64_t *h = ( const uint64_t * ) ( ( const char * ) src + sizeof( ovs_match_header ) );
  uint64_t *n = ( uint64_t * ) ( ( char * ) dst + sizeof( ovs_match_header ) );
  *n = htonll( *h );
}


static void
hton_ovs_match_64w( ovs_match_header *dst, const ovs_match_header *src ) {
  assert( dst != NULL );
  assert( src != NULL );

  hton_ovs_match_header( dst, src );

  const uint64_t *value_h = ( const uint64_t * ) ( ( const char * ) src + sizeof( ovs_match_header ) );
  uint64_t *value_n = ( uint64_t * ) ( ( char * ) dst + sizeof( ovs_match_header ) );
  *value_n = htonll( *value_h );

  const uint64_t *mask_h = ( const uint64_t * ) ( ( const char * ) value_h + sizeof( uint64_t ) );
  uint64_t *mask_n = ( uint64_t * ) ( ( char * ) value_n + sizeof( uint64_t ) );
  *mask_n = htonll( *mask_h );
}


void
hton_ovs_match_in_port( ovs_match_header *dst, const ovs_match_header *src ) {
  assert( dst != NULL );
  assert( src != NULL );
  assert( *src == OVSM_OF_IN_PORT );

  hton_ovs_match_16( dst, src );
}


void
hton_ovs_match_eth_addr( ovs_match_header *dst, const ovs_match_header *src ) {
  assert( dst != NULL );
  assert( src != NULL );
  assert( *src == OVSM_OF_ETH_SRC ||
#if OVS_VERSION_CODE >= OVS_VERSION( 1, 8, 0 )
          *src == OVSM_OF_ETH_SRC_W ||
#endif
          *src == OVSM_OF_ETH_DST || *src == OVSM_OF_ETH_DST_W ||
          *src == OVSM_OVS_ARP_SHA || *src == OVSM_OVS_ARP_THA ||
          *src == OVSM_OVS_ND_SLL || *src == OVSM_OVS_ND_TLL );

  uint8_t length = get_ovs_match_length( *src );
  hton_ovs_match_header( dst, src );
  memmove( ( char * ) dst + sizeof( ovs_match_header ), ( const char * ) src + sizeof( ovs_match_header ), length );
}


void
hton_ovs_match_eth_type( ovs_match_header *dst, const ovs_match_header *src ) {
  assert( dst != NULL );
  assert( src != NULL );
  assert( *src == OVSM_OF_ETH_TYPE );

  hton_ovs_match_16( dst, src );
}


void
hton_ovs_match_vlan_tci( ovs_match_header *dst, const ovs_match_header *src ) {
  assert( dst != NULL );
  assert( src != NULL );
  assert( *src == OVSM_OF_VLAN_TCI || *src == OVSM_OF_VLAN_TCI_W );

  if ( ovs_match_has_mask( *src ) ) {
    return hton_ovs_match_16w( dst, src );
  }

  hton_ovs_match_16( dst, src );
}


void
hton_ovs_match_ip_tos( ovs_match_header *dst, const ovs_match_header *src ) {
  assert( dst != NULL );
  assert( src != NULL );
  assert( *src == OVSM_OF_IP_TOS );

  hton_ovs_match_8( dst, src );
}


void
hton_ovs_match_ip_proto( ovs_match_header *dst, const ovs_match_header *src ) {
  assert( dst != NULL );
  assert( src != NULL );
  assert( *src == OVSM_OF_IP_PROTO );

  hton_ovs_match_8( dst, src );
}


void
hton_ovs_match_ip_addr( ovs_match_header *dst, const ovs_match_header *src ) {
  assert( dst != NULL );
  assert( src != NULL );
  assert( *src == OVSM_OF_IP_SRC || *src == OVSM_OF_IP_SRC_W ||
          *src == OVSM_OF_IP_DST || *src == OVSM_OF_IP_DST_W
#if OVS_VERSION_CODE >= OVS_VERSION( 2, 0, 0 )
          || *src == OVSM_OVS_TUN_IPV4_SRC || *src == OVSM_OVS_TUN_IPV4_SRC_W
          || *src == OVSM_OVS_TUN_IPV4_DST || *src == OVSM_OVS_TUN_IPV4_DST_W
#endif
        );

  if ( ovs_match_has_mask( *src ) ) {
    return hton_ovs_match_32w( dst, src );
  }

  hton_ovs_match_32( dst, src );
}


void
hton_ovs_match_ipv6_addr( ovs_match_header *dst, const ovs_match_header *src ) {
  assert( dst != NULL );
  assert( src != NULL );
  assert( *src == OVSM_OVS_IPV6_SRC || *src == OVSM_OVS_IPV6_SRC_W ||
          *src == OVSM_OVS_IPV6_DST || *src == OVSM_OVS_IPV6_DST_W ||
          *src == OVSM_OVS_ND_TARGET
#if OVS_VERSION_CODE >= OVS_VERSION( 1, 7, 0 )
          || *src == OVSM_OVS_ND_TARGET_W
#endif
        );

  uint8_t length = get_ovs_match_length( *src );
  hton_ovs_match_header( dst, src );
  memmove( ( char * ) dst + sizeof( ovs_match_header ), ( const char * ) src + sizeof( ovs_match_header ), length );
}


void
hton_ovs_match_icmpv6_type( ovs_match_header *dst, const ovs_match_header *src ) {
  assert( dst != NULL );
  assert( src != NULL );
  assert( *src == OVSM_OVS_ICMPV6_TYPE );

  hton_ovs_match_8( dst, src );
}


void
hton_ovs_match_icmpv6_code( ovs_match_header *dst, const ovs_match_header *src ) {
  assert( dst != NULL );
  assert( src != NULL );
  assert( *src == OVSM_OVS_ICMPV6_CODE );

  hton_ovs_match_8( dst, src );
}


void
hton_ovs_match_nd_target( ovs_match_header *dst, const ovs_match_header *src ) {
  assert( dst != NULL );
  assert( src != NULL );
  assert( *src == OVSM_OVS_ND_TARGET
#if OVS_VERSION_CODE >= OVS_VERSION( 1, 7, 0 )
          || *src == OVSM_OVS_ND_TARGET_W
#endif
        );

  hton_ovs_match_ipv6_addr( dst, src );
}


void
hton_ovs_match_nd_ll( ovs_match_header *dst, const ovs_match_header *src ) {
  assert( dst != NULL );
  assert( src != NULL );
  assert( *src == OVSM_OVS_ND_SLL || *src == OVSM_OVS_ND_TLL );

  hton_ovs_match_eth_addr( dst, src );
}


void
hton_ovs_match_ip_frag( ovs_match_header *dst, const ovs_match_header *src ) {
  assert( dst != NULL );
  assert( src != NULL );
  assert( *src == OVSM_OVS_IP_FRAG || *src == OVSM_OVS_IP_FRAG_W );

  if ( ovs_match_has_mask( *src ) ) {
    return hton_ovs_match_8w( dst, src );
  }

  hton_ovs_match_8( dst, src );
}


void
hton_ovs_match_ipv6_label( ovs_match_header *dst, const ovs_match_header *src ) {
  assert( dst != NULL );
  assert( src != NULL );
  assert( *src == OVSM_OVS_IPV6_LABEL
#if OVS_VERSION_CODE >= OVS_VERSION( 2, 1, 0 )
          || *src == OVSM_OVS_IPV6_LABEL_W
#endif
        );
#if OVS_VERSION_CODE >= OVS_VERSION( 2, 1, 0 )
  if ( ovs_match_has_mask( *src ) ) {
    return hton_ovs_match_32w( dst, src );
  }
#endif

  hton_ovs_match_32( dst, src );
}


void
hton_ovs_match_ip_ecn( ovs_match_header *dst, const ovs_match_header *src ) {
  assert( dst != NULL );
  assert( src != NULL );
  assert( *src == OVSM_OVS_IP_ECN );

  hton_ovs_match_8( dst, src );
}


void
hton_ovs_match_ip_ttl( ovs_match_header *dst, const ovs_match_header *src ) {
  assert( dst != NULL );
  assert( src != NULL );
  assert( *src == OVSM_OVS_IP_TTL );

  hton_ovs_match_8( dst, src );
}


void
hton_ovs_match_tcp_port( ovs_match_header *dst, const ovs_match_header *src ) {
  assert( dst != NULL );
  assert( src != NULL );
  assert( *src == OVSM_OF_TCP_SRC || *src == OVSM_OF_TCP_DST
#if OVS_VERSION_CODE >= OVS_VERSION( 1, 6, 0 )
          || *src == OVSM_OF_TCP_SRC_W || *src == OVSM_OF_TCP_DST_W
#endif
        );
#if OVS_VERSION_CODE >= OVS_VERSION( 1, 6, 0 )
  if ( ovs_match_has_mask( *src ) ) {
    return hton_ovs_match_16w( dst, src );
  }
#endif

  hton_ovs_match_16( dst, src );
}


void
hton_ovs_match_udp_port( ovs_match_header *dst, const ovs_match_header *src ) {
  assert( dst != NULL );
  assert( src != NULL );
  assert( *src == OVSM_OF_UDP_SRC || *src == OVSM_OF_UDP_DST
#if OVS_VERSION_CODE >= OVS_VERSION( 1, 6, 0 )
          || *src == OVSM_OF_UDP_SRC_W || *src == OVSM_OF_UDP_DST_W
#endif
        );

#if OVS_VERSION_CODE >= OVS_VERSION( 1, 6, 0 )
  if ( ovs_match_has_mask( *src ) ) {
    return hton_ovs_match_16w( dst, src );
  }
#endif

  hton_ovs_match_16( dst, src );
}


void
hton_ovs_match_icmp_type( ovs_match_header *dst, const ovs_match_header *src ) {
  assert( dst != NULL );
  assert( src != NULL );
  assert( *src == OVSM_OF_ICMP_TYPE );

  hton_ovs_match_8( dst, src );
}


void
hton_ovs_match_icmp_code( ovs_match_header *dst, const ovs_match_header *src ) {
  assert( dst != NULL );
  assert( src != NULL );
  assert( *src == OVSM_OF_ICMP_CODE );

  hton_ovs_match_8( dst, src );
}


void
hton_ovs_match_arp_op( ovs_match_header *dst, const ovs_match_header *src ) {
  assert( dst != NULL );
  assert( src != NULL );
  assert( *src == OVSM_OF_ARP_OP );

  hton_ovs_match_16( dst, src );
}


void
hton_ovs_match_arp_pa( ovs_match_header *dst, const ovs_match_header *src ) {
  assert( dst != NULL );
  assert( src != NULL );
  assert( *src == OVSM_OF_ARP_SPA || *src == OVSM_OF_ARP_SPA_W ||
          *src == OVSM_OF_ARP_TPA || *src == OVSM_OF_ARP_TPA_W );

  if ( ovs_match_has_mask( *src ) ) {
    return hton_ovs_match_32w( dst, src );
  }

  hton_ovs_match_32( dst, src );
}


void
hton_ovs_match_arp_ha( ovs_match_header *dst, const ovs_match_header *src ) {
  assert( dst != NULL );
  assert( src != NULL );
  assert( *src == OVSM_OVS_ARP_SHA || *src == OVSM_OVS_ARP_THA );

  hton_ovs_match_eth_addr( dst, src );
}


void
hton_ovs_match_reg( ovs_match_header *dst, const ovs_match_header *src ) {
  assert( dst != NULL );
  assert( src != NULL );

  if ( ovs_match_has_mask( *src ) ) {
    return hton_ovs_match_32w( dst, src );
  }

  hton_ovs_match_32( dst, src );
}


void
hton_ovs_match_tun_id( ovs_match_header *dst, const ovs_match_header *src ) {
  assert( dst != NULL );
  assert( src != NULL );
  assert( *src == OVSM_OVS_TUN_ID || *src == OVSM_OVS_TUN_ID_W );

  if ( ovs_match_has_mask( *src ) ) {
    return hton_ovs_match_64w( dst, src );
  }

  hton_ovs_match_64( dst, src );
}


#if OVS_VERSION_CODE >= OVS_VERSION( 1, 5, 0 )
void
hton_ovs_match_cookie( ovs_match_header *dst, const ovs_match_header *src ) {
  assert( dst != NULL );
  assert( src != NULL );
  assert( *src == OVSM_OVS_COOKIE || *src == OVSM_OVS_COOKIE_W );

  if ( ovs_match_has_mask( *src ) ) {
    return hton_ovs_match_64w( dst, src );
  }

  hton_ovs_match_64( dst, src );
}
#endif

#if OVS_VERSION_CODE >= OVS_VERSION( 2, 0, 0 )
void hton_ovs_match_pkt_mark( ovs_match_header *dst, const ovs_match_header *src ) {
  assert( dst != NULL );
  assert( src != NULL );
  assert( *src == OVSM_OVS_PKT_MARK || *src == OVSM_OVS_PKT_MARK_W );

  if ( ovs_match_has_mask( *src ) ) {
    return hton_ovs_match_32w( dst, src );
  }

  hton_ovs_match_32( dst, src );
}
#endif

#if OVS_VERSION_CODE >= OVS_VERSION( 2, 1, 0 )
void hton_ovs_match_tcp_flags( ovs_match_header *dst, const ovs_match_header *src ) {
  assert( dst != NULL );
  assert( src != NULL );
  assert( *src == OVSM_OVS_TCP_FLAGS || *src == OVSM_OVS_TCP_FLAGS_W );

  if ( ovs_match_has_mask( *src ) ) {
    return hton_ovs_match_16w( dst, src );
  }

  hton_ovs_match_16( dst, src );
}
#endif

void
hton_ovs_match( ovs_match_header *dst, const ovs_match_header *src ) {
  assert( dst != NULL );
  assert( src != NULL );

  switch ( *src ) {
    case OVSM_OF_IN_PORT:
    {
      hton_ovs_match_in_port( dst, src );
    }
    break;

    case OVSM_OF_ETH_DST:
    case OVSM_OF_ETH_DST_W:
    case OVSM_OF_ETH_SRC:
#if OVS_VERSION_CODE >= OVS_VERSION( 1, 8, 0 )
    case OVSM_OF_ETH_SRC_W:
#endif
    {
      hton_ovs_match_eth_addr( dst, src );
    }
    break;

    case OVSM_OF_ETH_TYPE:
    {
      hton_ovs_match_eth_type( dst, src );
    }
    break;

    case OVSM_OF_VLAN_TCI:
    case OVSM_OF_VLAN_TCI_W:
    {
      hton_ovs_match_vlan_tci( dst, src );
    }
    break;

    case OVSM_OF_IP_TOS:
    {
      hton_ovs_match_ip_tos( dst, src );
    }
    break;

    case OVSM_OF_IP_PROTO:
    {
      hton_ovs_match_ip_proto( dst, src );
    }
    break;

    case OVSM_OF_IP_SRC:
    case OVSM_OF_IP_SRC_W:
    case OVSM_OF_IP_DST:
    case OVSM_OF_IP_DST_W:
    {
      hton_ovs_match_ip_addr( dst, src );
    }
    break;

    case OVSM_OF_TCP_SRC:
    case OVSM_OF_TCP_DST:
#if OVS_VERSION_CODE >= OVS_VERSION( 1, 6, 0 )
    case OVSM_OF_TCP_SRC_W:
    case OVSM_OF_TCP_DST_W:
#endif
    {
      hton_ovs_match_tcp_port( dst, src );
    }
    break;

    case OVSM_OF_UDP_SRC:
    case OVSM_OF_UDP_DST:
#if OVS_VERSION_CODE >= OVS_VERSION( 1, 6, 0 )
    case OVSM_OF_UDP_SRC_W:
    case OVSM_OF_UDP_DST_W:
#endif
    {
      hton_ovs_match_udp_port( dst, src );
    }
    break;

    case OVSM_OF_ICMP_TYPE:
    {
      hton_ovs_match_icmp_type( dst, src );
    }
    break;

    case OVSM_OF_ICMP_CODE:
    {
      hton_ovs_match_icmp_code( dst, src );
    }
    break;

    case OVSM_OF_ARP_OP:
    {
      hton_ovs_match_arp_op( dst, src );
    }
    break;

    case OVSM_OF_ARP_SPA:
    case OVSM_OF_ARP_SPA_W:
    case OVSM_OF_ARP_TPA:
    case OVSM_OF_ARP_TPA_W:
    {
      hton_ovs_match_arp_pa( dst, src );
    }
    break;

    case OVSM_OVS_REG0:
    case OVSM_OVS_REG0_W:
    case OVSM_OVS_REG1:
    case OVSM_OVS_REG1_W:
    case OVSM_OVS_REG2:
    case OVSM_OVS_REG2_W:
    case OVSM_OVS_REG3:
    case OVSM_OVS_REG3_W:
    case OVSM_OVS_REG4:
    case OVSM_OVS_REG4_W:
#if OVS_VERSION_CODE >= OVS_VERSION( 1, 7, 0 )
    case OVSM_OVS_REG5:
    case OVSM_OVS_REG5_W:
    case OVSM_OVS_REG6:
    case OVSM_OVS_REG6_W:
    case OVSM_OVS_REG7:
    case OVSM_OVS_REG7_W:
#endif
    {
      hton_ovs_match_reg( dst, src );
    }
    break;

    case OVSM_OVS_TUN_ID:
    case OVSM_OVS_TUN_ID_W:
    {
      hton_ovs_match_tun_id( dst, src );
    }
    break;

    case OVSM_OVS_ARP_SHA:
    case OVSM_OVS_ARP_THA:
    {
      hton_ovs_match_arp_ha( dst, src );
    }
    break;

    case OVSM_OVS_IPV6_SRC:
    case OVSM_OVS_IPV6_SRC_W:
    case OVSM_OVS_IPV6_DST:
    case OVSM_OVS_IPV6_DST_W:
    {
      hton_ovs_match_ipv6_addr( dst, src );
    }
    break;

    case OVSM_OVS_ICMPV6_TYPE:
    {
      hton_ovs_match_icmpv6_type( dst, src );
    }
    break;

    case OVSM_OVS_ICMPV6_CODE:
    {
      hton_ovs_match_icmpv6_code( dst, src );
    }
    break;

    case OVSM_OVS_ND_TARGET:
#if OVS_VERSION_CODE >= OVS_VERSION( 1, 7, 0 )
    case OVSM_OVS_ND_TARGET_W:
#endif
    {
      hton_ovs_match_nd_target( dst, src );
    }
    break;

    case OVSM_OVS_ND_SLL:
    case OVSM_OVS_ND_TLL:
    {
      hton_ovs_match_nd_ll( dst, src );
    }
    break;

    case OVSM_OVS_IP_FRAG:
    case OVSM_OVS_IP_FRAG_W:
    {
      hton_ovs_match_ip_frag( dst, src );
    }
    break;

    case OVSM_OVS_IPV6_LABEL:
#if OVS_VERSION_CODE >= OVS_VERSION( 2, 1, 0 )
    case OVSM_OVS_IPV6_LABEL_W:
#endif
    {
      hton_ovs_match_ipv6_label( dst, src );
    }
    break;

    case OVSM_OVS_IP_ECN:
    {
      hton_ovs_match_ip_ecn( dst, src );
    }
    break;

    case OVSM_OVS_IP_TTL: 
    {
      hton_ovs_match_ip_ttl( dst, src );
    }
    break;

#if OVS_VERSION_CODE >= OVS_VERSION( 1, 5, 0 )
    case OVSM_OVS_COOKIE:
    case OVSM_OVS_COOKIE_W:
    {
      hton_ovs_match_cookie( dst, src );
    }
    break;
#endif

#if OVS_VERSION_CODE >= OVS_VERSION( 2, 0, 0 )
    case OVSM_OVS_TUN_IPV4_SRC:
    case OVSM_OVS_TUN_IPV4_SRC_W:
    case OVSM_OVS_TUN_IPV4_DST:
    case OVSM_OVS_TUN_IPV4_DST_W:
    {
      hton_ovs_match_ip_addr( dst, src );
    }
    break;

    case OVSM_OVS_PKT_MARK:
    case OVSM_OVS_PKT_MARK_W:
    {
      hton_ovs_match_pkt_mark( dst, src );
    }
    break;
#endif

#if OVS_VERSION_CODE >= OVS_VERSION( 2, 1, 0 )
    case OVSM_OVS_TCP_FLAGS:
    case OVSM_OVS_TCP_FLAGS_W:
    {
      hton_ovs_match_tcp_flags( dst, src );
    }
    break;
#endif

    default:
    {
      error( "Undefined match type ( header = %#x, type = %#x, has_mask = %u, length = %u ).",
             *src, get_ovs_match_type( *src ), ovs_match_has_mask( *src ), get_ovs_match_length( *src ) );
    }
    break;
  }

}


#define ntoh_ovs_match_header hton_ovs_match_header
#define ntoh_ovs_match_8 hton_ovs_match_8
#define ntoh_ovs_match_8w hton_ovs_match_8w
#define ntoh_ovs_match_16 hton_ovs_match_16
#define ntoh_ovs_match_16w hton_ovs_match_16w
#define ntoh_ovs_match_32 hton_ovs_match_32
#define ntoh_ovs_match_32w hton_ovs_match_32w
#define ntoh_ovs_match_64 hton_ovs_match_64
#define ntoh_ovs_match_64w hton_ovs_match_64w


void
ntoh_ovs_match_in_port( ovs_match_header *dst, const ovs_match_header *src ) {
  assert( dst != NULL );
  assert( src != NULL );

  ntoh_ovs_match_16( dst, src );
  assert( *dst == OVSM_OF_IN_PORT );
}


void
ntoh_ovs_match_eth_addr( ovs_match_header *dst, const ovs_match_header *src ) {
  assert( dst != NULL );
  assert( src != NULL );

  ntoh_ovs_match_header( dst, src );
  assert( *dst == OVSM_OF_ETH_DST ||
#if OVS_VERSION_CODE >= OVS_VERSION( 1, 8, 0 )
          *dst == OVSM_OF_ETH_DST_W ||
#endif
          *dst == OVSM_OF_ETH_DST || *dst == OVSM_OF_ETH_DST_W ||
          *dst == OVSM_OVS_ARP_SHA || *dst == OVSM_OVS_ARP_THA ||
          *dst == OVSM_OVS_ND_SLL || *dst == OVSM_OVS_ND_TLL );
  uint8_t length = get_ovs_match_length( *dst );

  memmove( ( char * ) dst + sizeof( ovs_match_header ), ( const char * ) src + sizeof( ovs_match_header ), length );
}


void
ntoh_ovs_match_eth_type( ovs_match_header *dst, const ovs_match_header *src ) {
  assert( dst != NULL );
  assert( src != NULL );

  ntoh_ovs_match_16( dst, src );
  assert( *dst == OVSM_OF_ETH_TYPE );
}


void
ntoh_ovs_match_vlan_tci( ovs_match_header *dst, const ovs_match_header *src ) {
  assert( dst != NULL );
  assert( src != NULL );

  if ( ovs_match_has_mask( ntohl( *src ) ) ) {
    ntoh_ovs_match_16w( dst, src );
    assert( *dst == OVSM_OF_VLAN_TCI_W );
    return;
  }

  ntoh_ovs_match_16( dst, src );
  assert( *dst == OVSM_OF_VLAN_TCI );
}


void
ntoh_ovs_match_ip_tos( ovs_match_header *dst, const ovs_match_header *src ) {
  assert( dst != NULL );
  assert( src != NULL );

  ntoh_ovs_match_8( dst, src );
  assert( *dst == OVSM_OF_IP_TOS );
}


void
ntoh_ovs_match_ip_proto( ovs_match_header *dst, const ovs_match_header *src ) {
  assert( dst != NULL );
  assert( src != NULL );

  ntoh_ovs_match_8( dst, src );
  assert( *dst == OVSM_OF_IP_PROTO );
}


void
ntoh_ovs_match_ip_addr( ovs_match_header *dst, const ovs_match_header *src ) {
  assert( dst != NULL );
  assert( src != NULL );

  if ( ovs_match_has_mask( ntohl( *src ) ) ) {
    ntoh_ovs_match_32w( dst, src );
    assert( *dst == OVSM_OF_IP_SRC_W || *dst == OVSM_OF_IP_DST_W
#if OVS_VERSION_CODE >= OVS_VERSION( 2, 0, 0 )
            || *dst == OVSM_OVS_TUN_IPV4_SRC_W || *dst == OVSM_OVS_TUN_IPV4_DST_W
#endif
          );
    return;
  }

  ntoh_ovs_match_32( dst, src );
  assert( *dst == OVSM_OF_IP_SRC || *dst == OVSM_OF_IP_DST
#if OVS_VERSION_CODE >= OVS_VERSION( 2, 0, 0 )
          || *dst == OVSM_OVS_TUN_IPV4_SRC || *dst == OVSM_OVS_TUN_IPV4_DST
#endif
        );
}


void
ntoh_ovs_match_ipv6_addr( ovs_match_header *dst, const ovs_match_header *src ) {
  assert( dst != NULL );
  assert( src != NULL );

  ntoh_ovs_match_header( dst, src );
  assert( *dst == OVSM_OVS_IPV6_SRC || *dst == OVSM_OVS_IPV6_SRC_W ||
          *dst == OVSM_OVS_IPV6_DST || *dst == OVSM_OVS_IPV6_DST_W ||
          *dst == OVSM_OVS_ND_TARGET
#if OVS_VERSION_CODE >= OVS_VERSION( 1, 7, 0 )
          || *dst == OVSM_OVS_ND_TARGET_W
#endif
        );
  uint8_t length = get_ovs_match_length( *dst );

  memmove( ( char * ) dst + sizeof( ovs_match_header ), ( const char * ) src + sizeof( ovs_match_header ), length );
}


void
ntoh_ovs_match_icmpv6_type( ovs_match_header *dst, const ovs_match_header *src ) {
  assert( dst != NULL );
  assert( src != NULL );

  ntoh_ovs_match_8( dst, src );
  assert( *dst == OVSM_OVS_ICMPV6_TYPE );
}


void
ntoh_ovs_match_icmpv6_code( ovs_match_header *dst, const ovs_match_header *src ) {
  assert( dst != NULL );
  assert( src != NULL );

  ntoh_ovs_match_8( dst, src );
  assert( *dst == OVSM_OVS_ICMPV6_CODE );
}


void
ntoh_ovs_match_nd_target( ovs_match_header *dst, const ovs_match_header *src ) {
  assert( dst != NULL );
  assert( src != NULL );

  ntoh_ovs_match_ipv6_addr( dst, src );
  assert( *dst == OVSM_OVS_ND_TARGET
#if OVS_VERSION_CODE >= OVS_VERSION( 1, 7, 0 )
          || *dst == OVSM_OVS_ND_TARGET_W
#endif
        );
}


void
ntoh_ovs_match_nd_ll( ovs_match_header *dst, const ovs_match_header *src ) {
  assert( dst != NULL );
  assert( src != NULL );

  ntoh_ovs_match_eth_addr( dst, src );
  assert( *dst == OVSM_OVS_ND_SLL || *dst == OVSM_OVS_ND_TLL );
}


void
ntoh_ovs_match_ip_frag( ovs_match_header *dst, const ovs_match_header *src ) {
  assert( dst != NULL );
  assert( src != NULL );

  if ( ovs_match_has_mask( ntohl( *src ) ) ) {
    ntoh_ovs_match_8w( dst, src );
    assert( *dst == OVSM_OVS_IP_FRAG_W );
    return;
  }

  ntoh_ovs_match_8( dst, src );
  assert( *dst == OVSM_OVS_IP_FRAG );
}


void
ntoh_ovs_match_ipv6_label( ovs_match_header *dst, const ovs_match_header *src ) {
  assert( dst != NULL );
  assert( src != NULL );

#if OVS_VERSION_CODE >= OVS_VERSION( 2, 1, 0 )
  if ( ovs_match_has_mask( ntohl( *src ) ) ) {
    ntoh_ovs_match_32w( dst, src );
    assert( *dst == OVSM_OVS_IPV6_LABEL_W );
    return;
  }
#endif
  ntoh_ovs_match_32( dst, src );
  assert( *dst == OVSM_OVS_IPV6_LABEL );
}


void
ntoh_ovs_match_ip_ecn( ovs_match_header *dst, const ovs_match_header *src ) {
  assert( dst != NULL );
  assert( src != NULL );

  ntoh_ovs_match_8( dst, src );
  assert( *dst == OVSM_OVS_IP_ECN );
}


void
ntoh_ovs_match_ip_ttl( ovs_match_header *dst, const ovs_match_header *src ) {
  assert( dst != NULL );
  assert( src != NULL );

  ntoh_ovs_match_8( dst, src );
  assert( *dst == OVSM_OVS_IP_TTL );
}


void
ntoh_ovs_match_tcp_port( ovs_match_header *dst, const ovs_match_header *src ) {
  assert( dst != NULL );
  assert( src != NULL );

#if OVS_VERSION_CODE >= OVS_VERSION( 1, 6, 0 )
  if ( ovs_match_has_mask( ntohl( *src ) ) ) {
    ntoh_ovs_match_16w( dst, src );
    assert( *dst == OVSM_OF_TCP_SRC_W || *dst == OVSM_OF_TCP_DST_W );
    return;
  }
#endif
  ntoh_ovs_match_16( dst, src );
  assert( *dst == OVSM_OF_TCP_SRC || *dst == OVSM_OF_TCP_DST );
}


void
ntoh_ovs_match_udp_port( ovs_match_header *dst, const ovs_match_header *src ) {
  assert( dst != NULL );
  assert( src != NULL );

#if OVS_VERSION_CODE >= OVS_VERSION( 1, 6, 0 )
  if ( ovs_match_has_mask( ntohl( *src ) ) ) {
    ntoh_ovs_match_16w( dst, src );
    assert( *dst == OVSM_OF_UDP_SRC_W || *dst == OVSM_OF_UDP_DST_W );
    return;
  }
#endif
  ntoh_ovs_match_16( dst, src );
  assert( *dst == OVSM_OF_UDP_SRC || *dst == OVSM_OF_UDP_DST );
}


void
ntoh_ovs_match_icmp_type( ovs_match_header *dst, const ovs_match_header *src ) {
  assert( dst != NULL );
  assert( src != NULL );

  ntoh_ovs_match_8( dst, src );
  assert( *dst == OVSM_OF_ICMP_TYPE );
}


void
ntoh_ovs_match_icmp_code( ovs_match_header *dst, const ovs_match_header *src ) {
  assert( dst != NULL );
  assert( src != NULL );

  ntoh_ovs_match_8( dst, src );
  assert( *dst == OVSM_OF_ICMP_CODE );
}


void
ntoh_ovs_match_arp_op( ovs_match_header *dst, const ovs_match_header *src ) {
  assert( dst != NULL );
  assert( src != NULL );

  ntoh_ovs_match_16( dst, src );
  assert( *dst == OVSM_OF_ARP_OP );
}


void
ntoh_ovs_match_arp_pa( ovs_match_header *dst, const ovs_match_header *src ) {
  assert( dst != NULL );
  assert( src != NULL );

  if ( ovs_match_has_mask( ntohl( *src ) ) ) {
    ntoh_ovs_match_32w( dst, src );
    assert( *dst == OVSM_OF_ARP_SPA_W || *dst == OVSM_OF_ARP_TPA_W );
    return;
  }

  ntoh_ovs_match_32( dst, src );
  assert( *dst == OVSM_OF_ARP_SPA || *dst == OVSM_OF_ARP_TPA );
}


void
ntoh_ovs_match_arp_ha( ovs_match_header *dst, const ovs_match_header *src ) {
  assert( dst != NULL );
  assert( src != NULL );

  ntoh_ovs_match_eth_addr( dst, src );
  assert( *dst == OVSM_OVS_ARP_SHA || *dst == OVSM_OVS_ARP_THA );
}


void
ntoh_ovs_match_reg( ovs_match_header *dst, const ovs_match_header *src ) {
  assert( dst != NULL );
  assert( src != NULL );

  if ( ovs_match_has_mask( ntohl( *src ) ) ) {
    ntoh_ovs_match_32w( dst, src );
    return;
  }

  ntoh_ovs_match_32( dst, src );
}


void
ntoh_ovs_match_tun_id( ovs_match_header *dst, const ovs_match_header *src ) {
  assert( dst != NULL );
  assert( src != NULL );

  if ( ovs_match_has_mask( ntohl( *src ) ) ) {
    ntoh_ovs_match_64w( dst, src );
    assert( *dst == OVSM_OVS_TUN_ID_W );
    return;
  }

  ntoh_ovs_match_64( dst, src );
  assert( *dst == OVSM_OVS_TUN_ID );
}


#if OVS_VERSION_CODE >= OVS_VERSION( 1, 5, 0 )
void
ntoh_ovs_match_cookie( ovs_match_header *dst, const ovs_match_header *src ) {
  assert( dst != NULL );
  assert( src != NULL );

  if ( ovs_match_has_mask( ntohl( *src ) ) ) {
    ntoh_ovs_match_64w( dst, src );
    assert( *dst == OVSM_OVS_COOKIE_W );
    return;
  }

  ntoh_ovs_match_64( dst, src );
  assert( *dst == OVSM_OVS_COOKIE );
}
#endif

#if OVS_VERSION_CODE >= OVS_VERSION( 2, 0, 0 )
void
ntoh_ovs_match_pkt_mark( ovs_match_header *dst, const ovs_match_header *src ) {
  assert( dst != NULL );
  assert( src != NULL );

  if ( ovs_match_has_mask( ntohl( *src ) ) ) {
    ntoh_ovs_match_32w( dst, src );
    assert( *dst == OVSM_OVS_PKT_MARK_W );
    return;
  }

  ntoh_ovs_match_32( dst, src );
  assert( *dst == OVSM_OVS_PKT_MARK );
}
#endif

#if OVS_VERSION_CODE >= OVS_VERSION( 2, 1, 0 )
void
ntoh_ovs_match_tcp_flags( ovs_match_header *dst, const ovs_match_header *src ) {
  assert( dst != NULL );
  assert( src != NULL );

  if ( ovs_match_has_mask( ntohl( *src ) ) ) {
    ntoh_ovs_match_16w( dst, src );
    assert( *dst == OVSM_OVS_TCP_FLAGS_W );
    return;
  }

  ntoh_ovs_match_16( dst, src );
  assert( *dst == OVSM_OVS_TCP_FLAGS );
}
#endif

void
ntoh_ovs_match( ovs_match_header *dst, const ovs_match_header *src ) {
  assert( dst != NULL );
  assert( src != NULL );

  switch ( ntohl( *src ) ) {
    case OVSM_OF_IN_PORT:
    {
      ntoh_ovs_match_in_port( dst, src );
    }
    break;

    case OVSM_OF_ETH_DST:
    case OVSM_OF_ETH_DST_W:
    case OVSM_OF_ETH_SRC:
#if OVS_VERSION_CODE >= OVS_VERSION( 1, 8, 0 )
    case OVSM_OF_ETH_SRC_W:
#endif
    {
      ntoh_ovs_match_eth_addr( dst, src );
    }
    break;

    case OVSM_OF_ETH_TYPE:
    {
      ntoh_ovs_match_eth_type( dst, src );
    }
    break;

    case OVSM_OF_VLAN_TCI:
    case OVSM_OF_VLAN_TCI_W:
    {
      ntoh_ovs_match_vlan_tci( dst, src );
    }
    break;

    case OVSM_OF_IP_TOS:
    {
      ntoh_ovs_match_ip_tos( dst, src );
    }
    break;

    case OVSM_OF_IP_PROTO:
    {
      ntoh_ovs_match_ip_proto( dst, src );
    }
    break;

    case OVSM_OF_IP_SRC:
    case OVSM_OF_IP_SRC_W:
    case OVSM_OF_IP_DST:
    case OVSM_OF_IP_DST_W:
    {
      ntoh_ovs_match_ip_addr( dst, src );
    }
    break;

    case OVSM_OF_TCP_SRC:
    case OVSM_OF_TCP_DST:
#if OVS_VERSION_CODE >= OVS_VERSION( 1, 6, 0 )
    case OVSM_OF_TCP_SRC_W:
    case OVSM_OF_TCP_DST_W:
#endif
    {
      ntoh_ovs_match_tcp_port( dst, src );
    }
    break;

    case OVSM_OF_UDP_SRC:
    case OVSM_OF_UDP_DST:
#if OVS_VERSION_CODE >= OVS_VERSION( 1, 6, 0 )
    case OVSM_OF_UDP_SRC_W:
    case OVSM_OF_UDP_DST_W:
#endif
    {
      ntoh_ovs_match_udp_port( dst, src );
    }
    break;

    case OVSM_OF_ICMP_TYPE:
    {
      ntoh_ovs_match_icmp_type( dst, src );
    }
    break;

    case OVSM_OF_ICMP_CODE:
    {
      ntoh_ovs_match_icmp_code( dst, src );
    }
    break;

    case OVSM_OF_ARP_OP:
    {
      ntoh_ovs_match_arp_op( dst, src );
    }
    break;

    case OVSM_OF_ARP_SPA:
    case OVSM_OF_ARP_SPA_W:
    case OVSM_OF_ARP_TPA:
    case OVSM_OF_ARP_TPA_W:
    {
      ntoh_ovs_match_arp_pa( dst, src );
    }
    break;

    case OVSM_OVS_REG0:
    case OVSM_OVS_REG0_W:
    case OVSM_OVS_REG1:
    case OVSM_OVS_REG1_W:
    case OVSM_OVS_REG2:
    case OVSM_OVS_REG2_W:
    case OVSM_OVS_REG3:
    case OVSM_OVS_REG3_W:
    case OVSM_OVS_REG4:
    case OVSM_OVS_REG4_W:
#if OVS_VERSION_CODE >= OVS_VERSION( 1, 7, 0 )
    case OVSM_OVS_REG5:
    case OVSM_OVS_REG5_W:
    case OVSM_OVS_REG6:
    case OVSM_OVS_REG6_W:
    case OVSM_OVS_REG7:
    case OVSM_OVS_REG7_W:
#endif
    {
      ntoh_ovs_match_reg( dst, src );
    }
    break;

    case OVSM_OVS_TUN_ID:
    case OVSM_OVS_TUN_ID_W:
    {
      ntoh_ovs_match_tun_id( dst, src );
    }
    break;

    case OVSM_OVS_ARP_SHA:
    case OVSM_OVS_ARP_THA:
    {
      ntoh_ovs_match_arp_ha( dst, src );
    }
    break;

    case OVSM_OVS_IPV6_SRC:
    case OVSM_OVS_IPV6_SRC_W:
    case OVSM_OVS_IPV6_DST:
    case OVSM_OVS_IPV6_DST_W:
    {
      ntoh_ovs_match_ipv6_addr( dst, src );
    }
    break;

    case OVSM_OVS_ICMPV6_TYPE:
    {
      ntoh_ovs_match_icmpv6_type( dst, src );
    }
    break;

    case OVSM_OVS_ICMPV6_CODE:
    {
      ntoh_ovs_match_icmpv6_code( dst, src );
    }
    break;

    case OVSM_OVS_ND_TARGET:
#if OVS_VERSION_CODE >= OVS_VERSION( 1, 7, 0 )
    case OVSM_OVS_ND_TARGET_W:
#endif
    {
      ntoh_ovs_match_nd_target( dst, src );
    }
    break;

    case OVSM_OVS_ND_SLL:
    case OVSM_OVS_ND_TLL:
    {
      ntoh_ovs_match_nd_ll( dst, src );
    }
    break;

    case OVSM_OVS_IP_FRAG:
    case OVSM_OVS_IP_FRAG_W:
    {
      ntoh_ovs_match_ip_frag( dst, src );
    }
    break;

    case OVSM_OVS_IPV6_LABEL:
#if OVS_VERSION_CODE >= OVS_VERSION( 2, 1, 0 )
    case OVSM_OVS_IPV6_LABEL_W:
#endif
    {
      ntoh_ovs_match_ipv6_label( dst, src );
    }
    break;

    case OVSM_OVS_IP_ECN:
    {
      ntoh_ovs_match_ip_ecn( dst, src );
    }
    break;

    case OVSM_OVS_IP_TTL: 
    {
      ntoh_ovs_match_ip_ttl( dst, src );
    }
    break;

#if OVS_VERSION_CODE >= OVS_VERSION( 1, 5, 0 )
    case OVSM_OVS_COOKIE:
    case OVSM_OVS_COOKIE_W:
    {
      ntoh_ovs_match_cookie( dst, src );
    }
    break;
#endif

#if OVS_VERSION_CODE >= OVS_VERSION( 2, 0, 0 )
    case OVSM_OVS_TUN_IPV4_SRC:
    case OVSM_OVS_TUN_IPV4_SRC_W:
    case OVSM_OVS_TUN_IPV4_DST:
    case OVSM_OVS_TUN_IPV4_DST_W:
    {
      ntoh_ovs_match_ip_addr( dst, src );
    }
    break;

    case OVSM_OVS_PKT_MARK:
    case OVSM_OVS_PKT_MARK_W:
    {
      ntoh_ovs_match_pkt_mark( dst, src );
    }
    break;
#endif

#if OVS_VERSION_CODE >= OVS_VERSION( 2, 1, 0 )
    case OVSM_OVS_TCP_FLAGS:
    case OVSM_OVS_TCP_FLAGS_W:
    {
      ntoh_ovs_match_tcp_flags( dst, src );
    }
    break;
#endif

    default:
    {
      ovs_match_header header;
      ntoh_ovs_match_header( &header, src );
      error( "Undefined match type ( header = %#x, type = %#x, has_mask = %u, length = %u ).",
             header, get_ovs_match_type( header ), ovs_match_has_mask( header ), get_ovs_match_length( header ) );
    }
    break;
  }

}


void
hton_ovs_action_reg_load( ovs_action_reg_load *dst, const ovs_action_reg_load *src ) {
  assert( src != NULL );
  assert( dst != NULL );

  dst->type = htons( src->type );
  dst->len = htons( src->len );
  dst->vendor = htonl( src->vendor );
  dst->subtype = htons( src->subtype );
  dst->ofs_nbits = htons( src->ofs_nbits );
  dst->dst = htonl( src->dst );
  dst->value = htonll( src->value );
}


void
hton_ovs_action_note( ovs_action_note *dst, const ovs_action_note *src ) {
  assert( src != NULL );
  assert( dst != NULL );

  dst->type = htons( src->type );
  dst->len = htons( src->len );
  dst->vendor = htonl( src->vendor );
  dst->subtype = htons( src->subtype );
  memcpy( dst->note, src->note, src->len - offsetof( ovs_action_note, note ) );
}


void
hton_ovs_action_resubmit( ovs_action_resubmit *dst, const ovs_action_resubmit *src ) {
  assert( src != NULL );
  assert( dst != NULL );

  dst->type = htons( src->type );
  dst->len = htons( src->len );
  dst->vendor = htonl( src->vendor );
  dst->subtype = htons( src->subtype );
  dst->in_port = htons( src->in_port );
  dst->table = src->table;
  dst->pad[ 0 ] = dst->pad[ 1 ] = dst->pad[ 2 ] = 0;
}


static void
hton_ovs_action_learn_front( ovs_action_learn *dst, const ovs_action_learn *src ) {
  assert( src != NULL );
  assert( dst != NULL );

  dst->type = htons( src->type );
  dst->len = htons( src->len );
  dst->vendor = htonl( src->vendor );
  dst->subtype = htons( src->subtype );
  dst->idle_timeout = htons( src->idle_timeout );
  dst->hard_timeout = htons( src->hard_timeout );
  dst->priority = htons( src->priority );
  dst->cookie = htonll( src->cookie );
  dst->flags = htons( src->flags );
  dst->table_id = src->table_id;
#if OVS_VERSION_CODE >= OVS_VERSION( 1, 6, 0 )
  dst->pad = 0;
  dst->fin_idle_timeout = htons( src->fin_idle_timeout );
  dst->fin_hard_timeout = htons( src->fin_hard_timeout );
#else
  dst->pad[ 0 ] = 0;
  dst->pad[ 1 ] = 0; dst->pad[ 2 ] = 0;
  dst->pad[ 3 ] = 0; dst->pad[ 4 ] = 0;
#endif
}


static void
hton_ovs_flow_mod_spec( uint16_t header, char *dst, const char *src ) {
  assert( src != NULL );
  assert( dst != NULL );

  *( uint16_t * ) dst = htons( *( const uint16_t * ) src );
  src += OVS_LEARN_HEADER_LENGTH;
  dst += OVS_LEARN_HEADER_LENGTH;

  if ( ( header & OVS_LEARN_SRC_MASK ) == OVS_LEARN_SRC_IMMEDIATE ) {
    int src_len = OVS_LEARN_SRC_IMMEDIATE_LENGTH( header );
    switch ( src_len ) {
      case sizeof( uint16_t ):
        *( uint16_t * ) dst = htons( *( const uint16_t * ) src );
        break;
      case sizeof( uint32_t ):
        *( uint32_t * ) dst = htonl( *( const uint32_t * ) src );
        break;
      case sizeof( uint64_t ):
        *( uint64_t * ) dst = htonll( *( const uint64_t * ) src );
        break;
      default:
        memcpy( dst, src, ( size_t ) src_len ); // network byte order
        break;
    }
    src += src_len;
    dst += src_len;
  }
  else {
    *( uint32_t * ) dst = htonl( *( const uint32_t * ) src );
    src += OVS_LEARN_MATCH_LENGTH;
    dst += OVS_LEARN_MATCH_LENGTH;
    *( uint16_t * ) dst = htons( *( const uint16_t * ) src );
    src += OVS_LEARN_OFS_LENGTH;
    dst += OVS_LEARN_OFS_LENGTH;
  }

  if ( ( header & OVS_LEARN_DST_MASK ) != OVS_LEARN_DST_OUTPUT ) {
    *( uint32_t * ) dst = htonl( *( const uint32_t * ) src );
    src += OVS_LEARN_MATCH_LENGTH;
    dst += OVS_LEARN_MATCH_LENGTH;
    *( uint16_t * ) dst = htons( *( const uint16_t * ) src );
    src += OVS_LEARN_OFS_LENGTH;
    dst += OVS_LEARN_OFS_LENGTH;
  }
}


void
hton_ovs_action_learn( ovs_action_learn *dst, const ovs_action_learn *src ) {
  assert( src != NULL );
  assert( dst != NULL );

  hton_ovs_action_learn_front( dst, src );
  int length = src->len - ( int ) sizeof( ovs_action_learn );
  const char *src_spec = ( const char * ) src + sizeof( ovs_action_learn );
  char *dst_spec = ( char * ) dst + sizeof( ovs_action_learn );
  memset( dst_spec, 0, ( size_t ) length );
  while ( length > ( int ) sizeof( uint16_t ) ) {
    uint16_t header = *( const uint16_t * ) src_spec;
    int spec_len = ovs_flow_mod_spec_length( header );
    if ( length < spec_len ) {
      break;
    }
    hton_ovs_flow_mod_spec( header, dst_spec, src_spec );
    length -= spec_len;
    src_spec = src_spec + spec_len;
    dst_spec = dst_spec + spec_len;
  }
}


void
hton_ovs_action( ovs_action_header *dst, const ovs_action_header *src ) {
  assert( src != NULL );
  assert( dst != NULL );
  assert( src->type == OFPAT_VENDOR );
  assert( src->vendor == OVS_VENDOR_ID );

  switch ( src->subtype ) {
    case OVSAST_RESUBMIT:
    {
      hton_ovs_action_resubmit( ( ovs_action_resubmit * ) dst,
                                ( const ovs_action_resubmit * ) src );
    }
    break;

    case OVSAST_REG_LOAD:
    {
      hton_ovs_action_reg_load( ( ovs_action_reg_load * ) dst,
                                ( const ovs_action_reg_load * ) src );
    }
    break;

    case OVSAST_NOTE:
    {
      hton_ovs_action_note( ( ovs_action_note * ) dst,
                            ( const ovs_action_note * ) src );
    }
    break;

    case OVSAST_RESUBMIT_TABLE:
    {
      hton_ovs_action_resubmit_table( ( ovs_action_resubmit * ) dst,
                                      ( const ovs_action_resubmit * ) src );
    }
    break;

    case OVSAST_LEARN:
    {
      hton_ovs_action_learn( ( ovs_action_learn * ) dst,
                             ( const ovs_action_learn * ) src );
    }
    break;

    default:
    {
      error( "Unsupported or undefined action ( subtype = %#x ).", src->subtype );
    }
    break;
  }
}


#define ntoh_ovs_action_learn_front hton_ovs_action_learn_front
#define ntoh_ovs_flow_mod_spec hton_ovs_flow_mod_spec


void
ntoh_ovs_action_note( ovs_action_note *dst, const ovs_action_note *src ) {
  assert( src != NULL );
  assert( dst != NULL );

  dst->type = ntohs( src->type );
  dst->len = ntohs( src->len );
  dst->vendor = ntohl( src->vendor );
  dst->subtype = ntohs( src->subtype );
  memcpy( dst->note, src->note, dst->len - offsetof( ovs_action_note, note ) );
}


void
ntoh_ovs_action_learn( ovs_action_learn *dst, const ovs_action_learn *src ) {
  assert( src != NULL );
  assert( dst != NULL );

  ntoh_ovs_action_learn_front( dst, src );
  int length = dst->len - ( int ) sizeof( ovs_action_learn );
  const char *src_spec = ( const char * ) src + sizeof( ovs_action_learn );
  char *dst_spec = ( char * ) dst + sizeof( ovs_action_learn );
  memset( dst_spec, 0, ( size_t ) length );
  while ( length > ( int ) sizeof( uint16_t ) ) {
    uint16_t header = ntohs( *( const uint16_t * ) src_spec );
    int spec_len = ovs_flow_mod_spec_length( header );
    if ( length < spec_len ) {
      break;
    }
    ntoh_ovs_flow_mod_spec( header, dst_spec, src_spec );
    length -= spec_len;
    src_spec = src_spec + spec_len;
    dst_spec = dst_spec + spec_len;
  }
}


void
ntoh_ovs_action( ovs_action_header *dst, const ovs_action_header *src ) {
  assert( src != NULL );
  assert( dst != NULL );
  assert( ntohs( src->type ) == OFPAT_VENDOR );
  assert( ntohl( src->vendor ) == OVS_VENDOR_ID );

  uint16_t subtype = ntohs( src->subtype );
  switch ( subtype ) {
    case OVSAST_RESUBMIT:
    {
      ntoh_ovs_action_resubmit( ( ovs_action_resubmit * ) dst,
                                ( const ovs_action_resubmit * ) src );
    }
    break;

    case OVSAST_REG_LOAD:
    {
      ntoh_ovs_action_reg_load( ( ovs_action_reg_load * ) dst,
                                ( const ovs_action_reg_load * ) src );
    }
    break;

    case OVSAST_NOTE:
    {
      ntoh_ovs_action_note( ( ovs_action_note * ) dst,
                            ( const ovs_action_note * ) src );
    }
    break;

    case OVSAST_RESUBMIT_TABLE:
    {
      ntoh_ovs_action_resubmit_table( ( ovs_action_resubmit * ) dst,
                                      ( const ovs_action_resubmit * ) src );
    }
    break;

    case OVSAST_LEARN:
    {
      ntoh_ovs_action_learn( ( ovs_action_learn * ) dst,
                             ( const ovs_action_learn * ) src );
    }
    break;

    default:
    {
      error( "Unsupported or undefined action ( subtype = %#x ).", subtype );
    }
    break;
  }
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
