/*
 * Byteorder converters
 *
 * Author: Yasunobu Chiba
 *
 * Copyright (C) 2012 NEC Corporation
 * NEC Confidential
 */


#ifndef OVS_BYTEORDER_H
#define OVS_BYTEORDER_H


#include <netinet/in.h>
#include "ovs_match.h"
#include "trema.h"


// Open vSwitch extended match
void hton_ovs_match_in_port( ovs_match_header *dst, const ovs_match_header *src );
void hton_ovs_match_eth_addr( ovs_match_header *dst, const ovs_match_header *src );
void hton_ovs_match_eth_type( ovs_match_header *dst, const ovs_match_header *src );
void hton_ovs_match_vlan_tci( ovs_match_header *dst, const ovs_match_header *src );
void hton_ovs_match_ip_tos( ovs_match_header *dst, const ovs_match_header *src );
void hton_ovs_match_ip_proto( ovs_match_header *dst, const ovs_match_header *src );
void hton_ovs_match_ip_addr( ovs_match_header *dst, const ovs_match_header *src );
void hton_ovs_match_ipv6_addr( ovs_match_header *dst, const ovs_match_header *src );
void hton_ovs_match_icmpv6_type( ovs_match_header *dst, const ovs_match_header *src );
void hton_ovs_match_icmpv6_code( ovs_match_header *dst, const ovs_match_header *src );
void hton_ovs_match_nd_target( ovs_match_header *dst, const ovs_match_header *src );
void hton_ovs_match_nd_sll( ovs_match_header *dst, const ovs_match_header *src );
void hton_ovs_match_nd_tll( ovs_match_header *dst, const ovs_match_header *src );
void hton_ovs_match_ip_frag( ovs_match_header *dst, const ovs_match_header *src );
void hton_ovs_match_ipv6_label( ovs_match_header *dst, const ovs_match_header *src );
void hton_ovs_match_ip_ecn( ovs_match_header *dst, const ovs_match_header *src );
void hton_ovs_match_ip_ttl( ovs_match_header *dst, const ovs_match_header *src );
void hton_ovs_match_tcp_port( ovs_match_header *dst, const ovs_match_header *src );
void hton_ovs_match_udp_port( ovs_match_header *dst, const ovs_match_header *src );
void hton_ovs_match_icmp_type( ovs_match_header *dst, const ovs_match_header *src );
void hton_ovs_match_icmp_code( ovs_match_header *dst, const ovs_match_header *src );
void hton_ovs_match_arp_op( ovs_match_header *dst, const ovs_match_header *src );
void hton_ovs_match_arp_pa( ovs_match_header *dst, const ovs_match_header *src );
void hton_ovs_match_arp_ha( ovs_match_header *dst, const ovs_match_header *src );
void hton_ovs_match_reg( ovs_match_header *dst, const ovs_match_header *src );
void hton_ovs_match_tun_id( ovs_match_header *dst, const ovs_match_header *src );
void hton_ovs_match( ovs_match_header *dst, const ovs_match_header *src );

void ntoh_ovs_match_in_port( ovs_match_header *dst, const ovs_match_header *src );
void ntoh_ovs_match_eth_addr( ovs_match_header *dst, const ovs_match_header *src );
void ntoh_ovs_match_eth_type( ovs_match_header *dst, const ovs_match_header *src );
void ntoh_ovs_match_vlan_tci( ovs_match_header *dst, const ovs_match_header *src );
void ntoh_ovs_match_ip_tos( ovs_match_header *dst, const ovs_match_header *src );
void ntoh_ovs_match_ip_proto( ovs_match_header *dst, const ovs_match_header *src );
void ntoh_ovs_match_ip_addr( ovs_match_header *dst, const ovs_match_header *src );
void ntoh_ovs_match_ipv6_addr( ovs_match_header *dst, const ovs_match_header *src );
void ntoh_ovs_match_icmpv6_type( ovs_match_header *dst, const ovs_match_header *src );
void ntoh_ovs_match_icmpv6_code( ovs_match_header *dst, const ovs_match_header *src );
void ntoh_ovs_match_nd_target( ovs_match_header *dst, const ovs_match_header *src );
void ntoh_ovs_match_nd_sll( ovs_match_header *dst, const ovs_match_header *src );
void ntoh_ovs_match_nd_tll( ovs_match_header *dst, const ovs_match_header *src );
void ntoh_ovs_match_ip_frag( ovs_match_header *dst, const ovs_match_header *src );
void ntoh_ovs_match_ipv6_label( ovs_match_header *dst, const ovs_match_header *src );
void ntoh_ovs_match_ip_ecn( ovs_match_header *dst, const ovs_match_header *src );
void ntoh_ovs_match_ip_ttl( ovs_match_header *dst, const ovs_match_header *src );
void ntoh_ovs_match_tcp_port( ovs_match_header *dst, const ovs_match_header *src );
void ntoh_ovs_match_udp_port( ovs_match_header *dst, const ovs_match_header *src );
void ntoh_ovs_match_icmp_type( ovs_match_header *dst, const ovs_match_header *src );
void ntoh_ovs_match_icmp_code( ovs_match_header *dst, const ovs_match_header *src );
void ntoh_ovs_match_arp_op( ovs_match_header *dst, const ovs_match_header *src );
void ntoh_ovs_match_arp_pa( ovs_match_header *dst, const ovs_match_header *src );
void ntoh_ovs_match_arp_ha( ovs_match_header *dst, const ovs_match_header *src );
void ntoh_ovs_match_reg( ovs_match_header *dst, const ovs_match_header *src );
void ntoh_ovs_match_tun_id( ovs_match_header *dst, const ovs_match_header *src );
void ntoh_ovs_match( ovs_match_header *dst, const ovs_match_header *src );

// Open vSwitch extended actions
void hton_ovs_action_reg_load( ovs_action_reg_load *dst, const ovs_action_reg_load *src );
void hton_ovs_action_resubmit( ovs_action_resubmit *dst, const ovs_action_resubmit *src );
#define hton_ovs_action_resubmit_table hton_ovs_action_resubmit
void hton_ovs_action( ovs_action_header *dst, const ovs_action_header *src );

#define ntoh_ovs_action_reg_load hton_ovs_action_reg_load
#define ntoh_ovs_action_resubmit hton_ovs_action_resubmit
#define ntoh_ovs_action_resubmit_table hton_ovs_action_resubmit
void ntoh_ovs_action( ovs_action_header *dst, const ovs_action_header *src );


#endif // OVS_BYTEORDER_H


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
