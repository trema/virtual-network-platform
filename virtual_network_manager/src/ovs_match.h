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


#ifndef OVS_MATCH_H
#define OVS_MATCH_H


#include <netinet/in.h>
#include "ovs_protocol.h"
#include "trema.h"


typedef uint32_t ovs_match_header;

typedef struct {
  int n_matches;
  list_element *list;
} ovs_matches;


uint32_t get_ovs_match_type( ovs_match_header header );
bool ovs_match_has_mask( ovs_match_header header );
uint8_t get_ovs_match_length( ovs_match_header header );
uint8_t get_ovs_match_reg_index( ovs_match_header header );

ovs_matches *create_ovs_matches();
bool delete_ovs_matches( ovs_matches *matches );
uint16_t get_ovs_matches_length( const ovs_matches *matches );
bool append_ovs_match_in_port( ovs_matches *matches, uint16_t in_port );
bool append_ovs_match_eth_src( ovs_matches *matches, uint8_t addr[ OFP_ETH_ALEN] );
bool append_ovs_match_eth_dst( ovs_matches *matches, uint8_t addr[ OFP_ETH_ALEN ], uint8_t mask[ OFP_ETH_ALEN ] );
bool append_ovs_match_eth_type( ovs_matches *matches, uint16_t type );
bool append_ovs_match_vlan_tci( ovs_matches *matches, uint16_t value, uint16_t mask );
bool append_ovs_match_ip_tos( ovs_matches *matches, uint8_t value );
bool append_ovs_match_ip_proto( ovs_matches *matches, uint8_t value );
bool append_ovs_match_ip_src( ovs_matches *matches, uint32_t addr, uint32_t mask );
bool append_ovs_match_ip_dst( ovs_matches *matches, uint32_t addr, uint32_t mask );
bool append_ovs_match_ipv6_src( ovs_matches *matches, struct in6_addr addr, struct in6_addr mask );
bool append_ovs_match_ipv6_dst( ovs_matches *matches, struct in6_addr addr, struct in6_addr mask );
bool append_ovs_match_icmpv6_type( ovs_matches *matches, uint8_t type );
bool append_ovs_match_icmpv6_code( ovs_matches *matches, uint8_t code );
bool append_ovs_match_nd_target( ovs_matches *matches, struct in6_addr addr );
bool append_ovs_match_nd_sll( ovs_matches *matches, uint8_t addr[ OFP_ETH_ALEN] );
bool append_ovs_match_nd_tll( ovs_matches *matches, uint8_t addr[ OFP_ETH_ALEN] );
bool append_ovs_match_ip_frag( ovs_matches *matches, uint8_t value, uint8_t mask );
bool append_ovs_match_ipv6_label( ovs_matches *matches, uint32_t value );
bool append_ovs_match_ip_ecn( ovs_matches *matches, uint8_t value );
bool append_ovs_match_ip_ttl( ovs_matches *matches, uint8_t value );
bool append_ovs_match_tcp_src( ovs_matches *matches, uint16_t port );
bool append_ovs_match_tcp_dst( ovs_matches *matches, uint16_t port );
bool append_ovs_match_udp_src( ovs_matches *matches, uint16_t port );
bool append_ovs_match_udp_dst( ovs_matches *matches, uint16_t port );
bool append_ovs_match_icmp_type( ovs_matches *matches, uint8_t type );
bool append_ovs_match_icmp_code( ovs_matches *matches, uint8_t code );
bool append_ovs_match_arp_op( ovs_matches *matches, uint16_t value );
bool append_ovs_match_arp_spa( ovs_matches *matches, uint32_t addr, uint32_t mask );
bool append_ovs_match_arp_tpa( ovs_matches *matches, uint32_t addr, uint32_t mask );
bool append_ovs_match_arp_sha( ovs_matches *matches, uint8_t addr[ OFP_ETH_ALEN] );
bool append_ovs_match_arp_tha( ovs_matches *matches, uint8_t addr[ OFP_ETH_ALEN] );
bool append_ovs_match_reg( ovs_matches *matches, uint8_t index, uint32_t value, uint32_t mask );
bool append_ovs_match_tun_id( ovs_matches *matches, uint64_t id, uint64_t mask );


#endif // OVS_MATCH_H


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
