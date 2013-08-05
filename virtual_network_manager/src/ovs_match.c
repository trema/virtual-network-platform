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
#include "ovs_match.h"
#include "trema.h"


uint32_t
get_ovs_match_type( ovs_match_header header ) {
  return ( uint32_t ) ( ( header >> 9 ) & 0x7fffff );
}


bool
ovs_match_has_mask( ovs_match_header header ) {
  return ( ( header >> 8 ) & 1 ) == 1 ? true : false;
}


uint8_t
get_ovs_match_length( ovs_match_header header ) {
  return ( uint8_t ) ( header & 0xff );
}


uint8_t
get_ovs_match_reg_index( ovs_match_header header ) {
  return ( uint8_t ) ( ( header >> 9 ) & 0x7f );
}


ovs_matches *
create_ovs_matches() {
  debug( "Creating an empty matches list." );

  ovs_matches *matches = xmalloc( sizeof( ovs_matches ) );

  if ( create_list( &matches->list ) == false ) {
    assert( 0 );
  }

  matches->n_matches = 0;

  return matches;
}


bool
delete_ovs_matches( ovs_matches *matches ) {
  assert( matches != NULL );

  debug( "Deleting an matches list ( n_matches = %d ).", matches->n_matches );

  list_element *element = matches->list;
  while ( element != NULL ) {
    xfree( element->data );
    element = element->next;
  }

  delete_list( matches->list );
  xfree( matches );

  return true;
}


uint16_t
get_ovs_matches_length( const ovs_matches *matches ) {
  assert( matches != NULL );

  debug( "Calculating the total length of matches." );

  int length = 0;
  list_element *match = matches->list;
  while ( match != NULL ) {
    ovs_match_header *header = match->data;
    length += ( int ) ( get_ovs_match_length( *header ) + sizeof( ovs_match_header ) );
    match = match->next;
  }

  debug( "Total length of matches = %d.", length );

  assert( length <= UINT16_MAX );

  return ( uint16_t ) length;

}


static bool
append_ovs_match( ovs_matches *matches, ovs_match_header *entry ) {
  assert( matches != NULL );
  assert( entry != NULL );

  bool ret = append_to_tail( &matches->list, entry );
  if ( ret ) {
    matches->n_matches++;
  }

  return ret;
}


static bool
append_ovs_match_8( ovs_matches *matches, ovs_match_header header, uint8_t value ) {
  assert( matches != NULL );
  assert( get_ovs_match_length( header ) == sizeof( uint8_t ) );

  ovs_match_header *buf = xmalloc( sizeof( ovs_match_header ) + sizeof( uint8_t ) );
  *buf = header;
  uint8_t *v = ( uint8_t * ) ( ( char * ) buf + sizeof( ovs_match_header ) );
  *v = value;

  return append_ovs_match( matches, buf );
}


static bool
append_ovs_match_8w( ovs_matches *matches, ovs_match_header header, uint8_t value, uint8_t mask ) {
  assert( matches != NULL );
  assert( get_ovs_match_length( header ) == ( sizeof( uint8_t ) * 2 ) );

  ovs_match_header *buf = xmalloc( sizeof( ovs_match_header ) + ( sizeof( uint8_t ) * 2 ) );
  *buf = header;
  uint8_t *v = ( uint8_t * ) ( ( char * ) buf + sizeof( ovs_match_header ) );
  *v = value;
  v = ( uint8_t * ) ( ( char * ) v + sizeof( uint8_t ) );
  *v = mask;

  return append_ovs_match( matches, buf );
}


static bool
append_ovs_match_16( ovs_matches *matches, ovs_match_header header, uint16_t value ) {
  assert( matches != NULL );
  assert( get_ovs_match_length( header ) == sizeof( uint16_t ) );

  ovs_match_header *buf = xmalloc( sizeof( ovs_match_header ) + sizeof( uint16_t ) );
  *buf = header;
  uint16_t *v = ( uint16_t * ) ( ( char * ) buf + sizeof( ovs_match_header ) );
  *v = value;

  return append_ovs_match( matches, buf );
}


static bool
append_ovs_match_16w( ovs_matches *matches, ovs_match_header header, uint16_t value, uint16_t mask ) {
  assert( matches != NULL );
  assert( get_ovs_match_length( header ) == ( sizeof( uint16_t ) * 2 ) );

  ovs_match_header *buf = xmalloc( sizeof( ovs_match_header ) + ( sizeof( uint16_t ) * 2 ) );
  *buf = header;
  uint16_t *v = ( uint16_t * ) ( ( char * ) buf + sizeof( ovs_match_header ) );
  *v = value;
  v = ( uint16_t * ) ( ( char * ) v + sizeof( uint16_t ) );
  *v = mask;

  return append_ovs_match( matches, buf );
}


static bool
append_ovs_match_32( ovs_matches *matches, ovs_match_header header, uint32_t value ) {
  assert( matches != NULL );
  assert( get_ovs_match_length( header ) == sizeof( uint32_t ) );

  ovs_match_header *buf = xmalloc( sizeof( ovs_match_header ) + sizeof( uint32_t ) );
  *buf = header;
  uint32_t *v = ( uint32_t * ) ( ( char * ) buf + sizeof( ovs_match_header ) );
  *v = value;

  return append_ovs_match( matches, buf );
}


static bool
append_ovs_match_32w( ovs_matches *matches, ovs_match_header header, uint32_t value, uint32_t mask ) {
  assert( matches != NULL );
  assert( get_ovs_match_length( header ) == ( sizeof( uint32_t ) * 2 ) );

  ovs_match_header *buf = xmalloc( sizeof( ovs_match_header ) + sizeof( uint32_t ) * 2 );
  *buf = header;
  uint32_t *v = ( uint32_t * ) ( ( char * ) buf + sizeof( ovs_match_header ) );
  *v = value;
  v = ( uint32_t * ) ( ( char * ) v + sizeof( uint32_t ) );
  *v = mask;

  return append_ovs_match( matches, buf );
}


static bool
append_ovs_match_64( ovs_matches *matches, ovs_match_header header, uint64_t value ) {
  assert( matches != NULL );
  assert( get_ovs_match_length( header ) == sizeof( uint64_t ) );

  ovs_match_header *buf = xmalloc( sizeof( ovs_match_header ) + sizeof( uint64_t ) );
  *buf = header;
  uint64_t *v = ( uint64_t * ) ( ( char * ) buf + sizeof( ovs_match_header ) );
  *v = value;

  return append_ovs_match( matches, buf );
}


static bool
append_ovs_match_64w( ovs_matches *matches, ovs_match_header header, uint64_t value, uint64_t mask ) {
  assert( matches != NULL );
  assert( get_ovs_match_length( header ) == ( sizeof( uint64_t ) * 2 ) );

  ovs_match_header *buf = xmalloc( sizeof( ovs_match_header ) + sizeof( uint64_t ) * 2 );
  *buf = header;
  uint64_t *v = ( uint64_t * ) ( ( char * ) buf + sizeof( ovs_match_header ) );
  *v = value;
  v = ( uint64_t * ) ( ( char * ) v + sizeof( uint64_t ) );
  *v = mask;

  return append_ovs_match( matches, buf );
}


static bool
append_ovs_match_eth_addr( ovs_matches *matches, ovs_match_header header, uint8_t addr[ OFP_ETH_ALEN ] ) {
  assert( matches != NULL );
  assert( get_ovs_match_length( header ) == ( OFP_ETH_ALEN * sizeof( uint8_t ) ) );

  ovs_match_header *buf = xmalloc( sizeof( ovs_match_header ) + ( OFP_ETH_ALEN * sizeof( uint8_t ) ) );
  *buf = header;
  uint8_t *value = ( uint8_t * ) ( ( char * ) buf + sizeof( ovs_match_header ) );
  memcpy( value, addr, OFP_ETH_ALEN * sizeof( uint8_t ) );

  return append_ovs_match( matches, buf );
}


static bool
append_ovs_match_eth_addr_w( ovs_matches *matches, ovs_match_header header,
                            uint8_t addr[ OFP_ETH_ALEN ], uint8_t mask[ OFP_ETH_ALEN ] ) {
  assert( matches != NULL );
  assert( get_ovs_match_length( header ) == ( 2 * OFP_ETH_ALEN * sizeof( uint8_t ) ) );

  ovs_match_header *buf = xmalloc( sizeof( ovs_match_header ) + ( 2 * OFP_ETH_ALEN * sizeof( uint8_t ) ) );
  *buf = header;
  uint8_t *value = ( uint8_t * ) ( ( char * ) buf + sizeof( ovs_match_header ) );
  memcpy( value, addr, OFP_ETH_ALEN * sizeof( uint8_t ) );
  value = ( uint8_t * ) ( ( char * ) value + ( sizeof( uint8_t ) * OFP_ETH_ALEN ) );
  memcpy( value, mask, OFP_ETH_ALEN * sizeof( uint8_t ) );

  return append_ovs_match( matches, buf );
}


bool append_ovs_match_in_port( ovs_matches *matches, uint16_t in_port ) {
  assert( matches != NULL );

  return append_ovs_match_16( matches, OVSM_OF_IN_PORT, in_port );
}


bool
append_ovs_match_eth_src( ovs_matches *matches, uint8_t addr[ OFP_ETH_ALEN ] ) {
  assert( matches != NULL );

  return append_ovs_match_eth_addr( matches, OVSM_OF_ETH_SRC, addr );
}


bool
append_ovs_match_eth_dst( ovs_matches *matches,
                         uint8_t addr[ OFP_ETH_ALEN ], uint8_t mask[ OFP_ETH_ALEN ] ) {
  assert( matches != NULL );

  uint8_t all_one[ OFP_ETH_ALEN ] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
  if ( memcmp( mask, all_one, OFP_ETH_ALEN ) == 0 ) {
    return append_ovs_match_eth_addr( matches, OVSM_OF_ETH_DST, addr );
  }

  return append_ovs_match_eth_addr_w( matches, OVSM_OF_ETH_DST_W, addr, mask );
}


bool
append_ovs_match_eth_type( ovs_matches *matches, uint16_t type ) {
  assert( matches != NULL );

  return append_ovs_match_16( matches, OVSM_OF_ETH_TYPE, type );
}


bool
append_ovs_match_vlan_tci( ovs_matches *matches, uint16_t value, uint16_t mask ) {
  assert( matches != NULL );

  if ( mask == UINT16_MAX ) {
    return append_ovs_match_16( matches, OVSM_OF_VLAN_TCI, value );
  }

  return append_ovs_match_16w( matches, OVSM_OF_VLAN_TCI_W, value, mask );
}


bool
append_ovs_match_ip_tos( ovs_matches *matches, uint8_t value ) {
  assert( matches != NULL );

  return append_ovs_match_8( matches, OVSM_OF_IP_TOS, value );
}


bool
append_ovs_match_ip_proto( ovs_matches *matches, uint8_t value ) {
  assert( matches != NULL );

  return append_ovs_match_8( matches, OVSM_OF_IP_PROTO, value );
}


bool
append_ovs_match_ip_src( ovs_matches *matches, uint32_t addr, uint32_t mask ) {
  assert( matches != NULL );

  if ( mask == UINT32_MAX ) {
    return append_ovs_match_32( matches, OVSM_OF_IP_SRC, addr );
  }

  return append_ovs_match_32w( matches, OVSM_OF_IP_SRC_W, addr, mask );
}


bool
append_ovs_match_ip_dst( ovs_matches *matches, uint32_t addr, uint32_t mask ) {
  assert( matches != NULL );

  if ( mask == UINT32_MAX ) {
    return append_ovs_match_32( matches, OVSM_OF_IP_DST, addr );
  }

  return append_ovs_match_32w( matches, OVSM_OF_IP_DST_W, addr, mask );
}


static bool
append_ovs_match_ipv6_addr( ovs_matches *matches, ovs_match_header header, struct in6_addr addr, struct in6_addr mask ) {
  assert( matches != NULL );

  uint8_t length = get_ovs_match_length( header );
  ovs_match_header *buf = xmalloc( sizeof( ovs_match_header ) + length );
  *buf = header;
  void *p = ( char * ) buf + sizeof( ovs_match_header );
  memcpy( p, &addr, sizeof( struct in6_addr ) );

  if ( ovs_match_has_mask( header ) ) {
    p = ( char * ) p + sizeof( struct in6_addr );
    memcpy( p, &mask, sizeof( struct in6_addr ) );
  }

  return append_ovs_match( matches, buf );
}


bool
append_ovs_match_ipv6_src( ovs_matches *matches, struct in6_addr addr, struct in6_addr mask ) {
  assert( matches != NULL );

  struct in6_addr all_one;
  memset( all_one.s6_addr, 0xff, sizeof( all_one.s6_addr ) );
  if ( memcmp( mask.s6_addr, all_one.s6_addr, sizeof( mask.s6_addr ) ) == 0 ) {
    return append_ovs_match_ipv6_addr( matches, OVSM_OVS_IPV6_SRC, addr, mask );
  }

  return append_ovs_match_ipv6_addr( matches, OVSM_OVS_IPV6_SRC_W, addr, mask );
}


bool
append_ovs_match_ipv6_dst( ovs_matches *matches, struct in6_addr addr, struct in6_addr mask ) {
  assert( matches != NULL );

  struct in6_addr all_one;
  memset( all_one.s6_addr, 0xff, sizeof( all_one.s6_addr ) );
  if ( memcmp( mask.s6_addr, all_one.s6_addr, sizeof( mask.s6_addr ) ) == 0 ) {
    return append_ovs_match_ipv6_addr( matches, OVSM_OVS_IPV6_DST, addr, mask );
  }

  return append_ovs_match_ipv6_addr( matches, OVSM_OVS_IPV6_DST_W, addr, mask );
}


bool
append_ovs_match_icmpv6_type( ovs_matches *matches, uint8_t type ) {
  assert( matches != NULL );

  return append_ovs_match_8( matches, OVSM_OVS_ICMPV6_TYPE, type );
}


bool
append_ovs_match_icmpv6_code( ovs_matches *matches, uint8_t code ) {
  assert( matches != NULL );

  return append_ovs_match_8( matches, OVSM_OVS_ICMPV6_CODE, code );
}


bool
append_ovs_match_nd_target( ovs_matches *matches, struct in6_addr addr ) {
  assert( matches != NULL );

  struct in6_addr mask = IN6ADDR_ANY_INIT;
  return append_ovs_match_ipv6_addr( matches, OVSM_OVS_ND_TARGET, addr, mask );
}


bool
append_ovs_match_nd_sll( ovs_matches *matches, uint8_t addr[ OFP_ETH_ALEN ] ) {
  assert( matches != NULL );

  return append_ovs_match_eth_addr( matches, OVSM_OVS_ND_SLL, addr );
}


bool
append_ovs_match_nd_tll( ovs_matches *matches, uint8_t addr[ OFP_ETH_ALEN ] ) {
  assert( matches != NULL );

  return append_ovs_match_eth_addr( matches, OVSM_OVS_ND_TLL, addr );
}


bool
append_ovs_match_ip_frag( ovs_matches *matches, uint8_t value, uint8_t mask ) {
  assert( matches != NULL );

  if ( mask == UINT8_MAX ) {
    return append_ovs_match_8( matches, OVSM_OVS_IP_FRAG, value );
  }

  return append_ovs_match_8w( matches, OVSM_OVS_IP_FRAG_W, value, mask );
}


bool
append_ovs_match_ipv6_label( ovs_matches *matches, uint32_t value ) {
  assert( matches != NULL );

  return append_ovs_match_32( matches, OVSM_OVS_IPV6_LABEL, value );
}


bool
append_ovs_match_ip_ecn( ovs_matches *matches, uint8_t value ) {
  assert( matches != NULL );

  return append_ovs_match_8( matches, OVSM_OVS_IP_ECN, value );
}


bool
append_ovs_match_ip_ttl( ovs_matches *matches, uint8_t value ) {
  assert( matches != NULL );

  return append_ovs_match_8( matches, OVSM_OVS_IP_TTL, value );
}


bool
append_ovs_match_tcp_src( ovs_matches *matches, uint16_t port ) {
  assert( matches != NULL );

  return append_ovs_match_16( matches, OVSM_OF_TCP_SRC, port );
}


bool
append_ovs_match_tcp_dst( ovs_matches *matches, uint16_t port ) {
  assert( matches != NULL );

  return append_ovs_match_16( matches, OVSM_OF_TCP_DST, port );
}


bool
append_ovs_match_udp_src( ovs_matches *matches, uint16_t port ) {
  assert( matches != NULL );

  return append_ovs_match_16( matches, OVSM_OF_UDP_SRC, port );
}


bool
append_ovs_match_udp_dst( ovs_matches *matches, uint16_t port ) {
  assert( matches != NULL );

  return append_ovs_match_16( matches, OVSM_OF_UDP_DST, port );
}


bool
append_ovs_match_icmp_type( ovs_matches *matches, uint8_t type ) {
  assert( matches != NULL );

  return append_ovs_match_8( matches, OVSM_OF_ICMP_TYPE, type );
}


bool
append_ovs_match_icmp_code( ovs_matches *matches, uint8_t code ) {
  assert( matches != NULL );

  return append_ovs_match_8( matches, OVSM_OF_ICMP_CODE, code );
}


bool
append_ovs_match_arp_op( ovs_matches *matches, uint16_t value ) {
  assert( matches != NULL );

  return append_ovs_match_16( matches, OVSM_OF_ARP_OP, value );  
}


bool
append_ovs_match_arp_spa( ovs_matches *matches, uint32_t addr, uint32_t mask ) {
  assert( matches != NULL );

  if ( mask == UINT32_MAX ) {
    return append_ovs_match_32( matches, OVSM_OF_ARP_SPA, addr );
  }

  return append_ovs_match_32w( matches, OVSM_OF_ARP_SPA_W, addr, mask );  
}


bool
append_ovs_match_arp_tpa( ovs_matches *matches, uint32_t addr, uint32_t mask ) {
  assert( matches != NULL );

  if ( mask == UINT32_MAX ) {
    return append_ovs_match_32( matches, OVSM_OF_ARP_TPA, addr );
  }

  return append_ovs_match_32w( matches, OVSM_OF_ARP_TPA_W, addr, mask );  
}


bool
append_ovs_match_arp_sha( ovs_matches *matches, uint8_t addr[ OFP_ETH_ALEN] ) {
  assert( matches != NULL );

  return append_ovs_match_eth_addr( matches, OVSM_OVS_ARP_SHA, addr );
}


bool
append_ovs_match_arp_tha( ovs_matches *matches, uint8_t addr[ OFP_ETH_ALEN] ) {
  assert( matches != NULL );

  return append_ovs_match_eth_addr( matches, OVSM_OVS_ARP_THA, addr );
}


bool
append_ovs_match_reg( ovs_matches *matches, uint8_t index, uint32_t value, uint32_t mask ) {
  assert( matches != NULL );

  if ( mask == UINT32_MAX ) {
    return append_ovs_match_32( matches, ( ovs_match_header ) OVSM_OVS_REG( index ), value );
  }

  return append_ovs_match_32w( matches, ( ovs_match_header ) OVSM_OVS_REG( index ), value, mask );
}


bool
append_ovs_match_tun_id( ovs_matches *matches, uint64_t id, uint64_t mask ) {
  assert( matches != NULL );

  if ( mask == UINT64_MAX ) {
    return append_ovs_match_64( matches, OVSM_OVS_TUN_ID, id );
  }

  return append_ovs_match_64w( matches, OVSM_OVS_TUN_ID_W, id, mask );
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
