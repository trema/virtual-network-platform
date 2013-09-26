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


#ifndef OVS_PROTOCOL_H
#define OVS_PROTOCOL_H


#include <stdint.h>
#include <openflow.h>


#define OVSM_HEADER( _vendor, _field, _length ) ( ( ( _vendor ) << 16 ) | ( ( _field ) << 9 ) | ( 0 << 8 ) | ( _length ) )
#define OVSM_HEADER_W( _vendor, _field, _length ) ( ( ( _vendor ) << 16 ) | ( ( _field ) << 9 ) | ( 1 << 8 ) | ( ( _length ) * 2 ) )
#define OVSM_OVS_REG( _index ) OVSM_HEADER( 0x0001, _index, 4 )
#define OVSM_OVS_REG_W( _index ) OVSM_HEADER_W( 0x0001, _index, 4 )


enum {
  OVSM_OF_IN_PORT = OVSM_HEADER( 0x0000, 0, 2 ),
  OVSM_OF_ETH_DST = OVSM_HEADER( 0x0000, 1, 6 ),
  OVSM_OF_ETH_DST_W = OVSM_HEADER_W( 0x0000, 1, 6 ),
  OVSM_OF_ETH_SRC = OVSM_HEADER( 0x0000, 2, 6 ),
  OVSM_OF_ETH_TYPE = OVSM_HEADER( 0x0000, 3, 2 ),
  OVSM_OF_VLAN_TCI = OVSM_HEADER( 0x0000, 4, 2 ),
  OVSM_OF_VLAN_TCI_W = OVSM_HEADER_W( 0x0000, 4, 2 ),
  OVSM_OF_IP_TOS = OVSM_HEADER( 0x0000, 5, 1 ),
  OVSM_OF_IP_PROTO = OVSM_HEADER( 0x0000, 6, 1 ),
  OVSM_OF_IP_SRC = OVSM_HEADER( 0x0000, 7, 4 ),
  OVSM_OF_IP_SRC_W = OVSM_HEADER_W( 0x0000, 7, 4 ),
  OVSM_OF_IP_DST = OVSM_HEADER( 0x0000, 8, 4 ),
  OVSM_OF_IP_DST_W = OVSM_HEADER_W( 0x0000, 8, 4 ),
  OVSM_OF_TCP_SRC = OVSM_HEADER( 0x0000, 9, 2 ),
  OVSM_OF_TCP_DST = OVSM_HEADER( 0x0000, 10, 2 ),
  OVSM_OF_UDP_SRC = OVSM_HEADER( 0x0000, 11, 2 ),
  OVSM_OF_UDP_DST = OVSM_HEADER( 0x0000, 12, 2 ),
  OVSM_OF_ICMP_TYPE = OVSM_HEADER( 0x0000, 13, 1 ),
  OVSM_OF_ICMP_CODE = OVSM_HEADER( 0x0000, 14, 1 ),
  OVSM_OF_ARP_OP = OVSM_HEADER( 0x0000, 15, 2 ),
  OVSM_OF_ARP_SPA = OVSM_HEADER( 0x0000, 16, 4 ),
  OVSM_OF_ARP_SPA_W = OVSM_HEADER_W( 0x0000, 16, 4 ),
  OVSM_OF_ARP_TPA = OVSM_HEADER( 0x0000, 17, 4 ),
  OVSM_OF_ARP_TPA_W = OVSM_HEADER_W( 0x0000, 17, 4 ),
  OVSM_OVS_REG0 = OVSM_HEADER( 0x0001, 0, 4 ),
  OVSM_OVS_REG0_W = OVSM_HEADER_W( 0x0001, 0, 4 ),
  OVSM_OVS_REG1 = OVSM_HEADER( 0x0001, 1, 4 ),
  OVSM_OVS_REG1_W = OVSM_HEADER_W( 0x0001, 1, 4 ),
  OVSM_OVS_REG2 = OVSM_HEADER( 0x0001, 2, 4 ),
  OVSM_OVS_REG2_W = OVSM_HEADER_W( 0x0001, 2, 4 ),
  OVSM_OVS_REG3 = OVSM_HEADER( 0x0001, 3, 4 ),
  OVSM_OVS_REG3_W = OVSM_HEADER_W( 0x0001, 3, 4 ),
  OVSM_OVS_REG4 = OVSM_HEADER( 0x0001, 4, 4 ),
  OVSM_OVS_REG4_W = OVSM_HEADER_W( 0x0001, 4, 4 ),
  OVSM_OVS_TUN_ID = OVSM_HEADER( 0x0001, 16, 8 ),
  OVSM_OVS_TUN_ID_W = OVSM_HEADER_W( 0x0001, 16, 8 ),
  OVSM_OVS_ARP_SHA = OVSM_HEADER( 0x0001, 17, 6 ),
  OVSM_OVS_ARP_THA = OVSM_HEADER( 0x0001, 18, 6 ),
  OVSM_OVS_IPV6_SRC = OVSM_HEADER( 0x0001, 19, 16 ),
  OVSM_OVS_IPV6_SRC_W = OVSM_HEADER_W( 0x0001, 19, 16 ),
  OVSM_OVS_IPV6_DST = OVSM_HEADER( 0x0001, 20, 16 ),
  OVSM_OVS_IPV6_DST_W = OVSM_HEADER_W( 0x0001, 20, 16 ),
  OVSM_OVS_ICMPV6_TYPE = OVSM_HEADER( 0x0001, 21, 1 ),
  OVSM_OVS_ICMPV6_CODE = OVSM_HEADER( 0x0001, 22, 1 ),
  OVSM_OVS_ND_TARGET = OVSM_HEADER( 0x0001, 23, 16 ),
  OVSM_OVS_ND_SLL = OVSM_HEADER( 0x0001, 24, 6 ),
  OVSM_OVS_ND_TLL = OVSM_HEADER( 0x0001, 25, 6 ),
  OVSM_OVS_IP_FRAG = OVSM_HEADER( 0x0001, 26, 1 ),
  OVSM_OVS_IP_FRAG_W = OVSM_HEADER_W( 0x0001, 26, 1 ),
  OVSM_OVS_IPV6_LABEL = OVSM_HEADER( 0x0001, 27, 4 ),
  OVSM_OVS_IP_ECN = OVSM_HEADER( 0x0001, 28, 1 ),
  OVSM_OVS_IP_TTL = OVSM_HEADER( 0x0001, 29, 1 ),
};


enum {
  OVS_VENDOR_ID = 0x00002320UL,
};


enum {
  OVSET_VENDOR = 0xb0c2U,
};


enum {
  OVSVC_VENDOR_ERROR = 0,
};


enum {
  OVST_SET_FLOW_FORMAT = 12,
  OVST_FLOW_MOD = 13,
  OVST_FLOW_REMOVED = 14,
  OVST_FLOW_MOD_TABLE_ID = 15,
};


enum {
  OVSAST_RESUBMIT = 1,
  OVSAST_REG_LOAD = 7,
  OVSAST_RESUBMIT_TABLE = 14,
  OVSAST_LEARN = 16,
};


enum {
  OVSFF_OPENFLOW10 = 0,
  OVSFF_OVSM = 2,
};


typedef struct {
  uint32_t vendor;
  uint16_t type;
  uint16_t code;
} ovs_vendor_error;


typedef struct {
  struct ofp_header header;
  uint32_t vendor;
  uint32_t subtype;
} ovs_header;


typedef struct {
  struct ofp_header header;
  uint32_t vendor;
  uint32_t subtype;
  uint8_t set;
  uint8_t pad[ 7 ];
} ovs_flow_mod_table_id;


typedef struct {
  uint16_t type;
  uint16_t len;
  uint32_t vendor;
  uint16_t subtype;
  uint8_t pad[ 6 ];
} ovs_action_header;


typedef struct {
  uint16_t type;
  uint16_t len;
  uint32_t vendor;
  uint16_t subtype;
  uint16_t in_port;
  uint8_t table;
  uint8_t pad[ 3 ];
} ovs_action_resubmit;


typedef struct {
  uint16_t type;
  uint16_t len;
  uint32_t vendor;
  uint16_t subtype;
  uint16_t ofs_nbits;
  uint32_t dst;
  uint64_t value;
} ovs_action_reg_load;


typedef struct {
  uint16_t type;
  uint16_t len;
  uint32_t vendor;
  uint16_t subtype;
  uint16_t idle_timeout;
  uint16_t hard_timeout;
  uint16_t priority;
  uint64_t cookie;
  uint16_t flags;
  uint8_t table_id;
  uint8_t pad[ 5 ];
} ovs_action_learn;


typedef struct {
  struct ofp_header header;
  uint32_t vendor;
  uint32_t subtype;
  uint32_t format;
} ovs_set_flow_format;


typedef struct {
  ovs_header ovsh;
  uint64_t cookie;
  uint16_t command;
  uint16_t idle_timeout;
  uint16_t hard_timeout;
  uint16_t priority;
  uint32_t buffer_id;
  uint16_t out_port;
  uint16_t flags;
  uint16_t match_len;
  uint8_t pad[ 6 ];
} ovs_flow_mod;


typedef struct {
  ovs_header ovsh;
  uint64_t cookie;
  uint16_t priority;
  uint8_t reason;
  uint8_t pad[ 1 ];
  uint32_t duration_sec;
  uint32_t duration_nsec;
  uint16_t idle_timeout;
  uint16_t match_len;
  uint64_t packet_count;
  uint64_t byte_count;
} ovs_flow_removed;


#define OVS_LEARN_N_BITS_MASK 0x3ff
#define OVS_LEARN_SRC_SHIFT 13
#define OVS_LEARN_SRC_FIELD     (0 << OVS_LEARN_SRC_SHIFT)
#define OVS_LEARN_SRC_IMMEDIATE (1 << OVS_LEARN_SRC_SHIFT)
#define OVS_LEARN_SRC_MASK      (1 << OVS_LEARN_SRC_SHIFT)
#define OVS_LEARN_DST_SHIFT 11
#define OVS_LEARN_DST_MATCH  (0 << OVS_LEARN_DST_SHIFT)
#define OVS_LEARN_DST_LOAD   (1 << OVS_LEARN_DST_SHIFT)
#define OVS_LEARN_DST_OUTPUT (2 << OVS_LEARN_DST_SHIFT)
#define OVS_LEARN_DST_MASK   (3 << OVS_LEARN_DST_SHIFT)


#define ROUND_UP_16( n ) ( ( ( n ) + 15 ) / 16 )
#define OVS_LEARN_HEADER_LENGTH ( ( int ) sizeof( uint16_t ) )
#define OVS_LEARN_SRC_IMMEDIATE_LENGTH( header ) ( 2 * ( ROUND_UP_16( ( header ) & OVS_LEARN_DST_OUTPUT ) ) )
#define OVS_LEARN_MATCH_LENGTH ( ( int ) sizeof( uint32_t ) )
#define OVS_LEARN_OFS_LENGTH ( ( int ) sizeof( uint16_t ) )


static inline uint16_t ovs_flow_mod_spec_length( uint16_t header ) {
  int length = 0;
  length += OVS_LEARN_HEADER_LENGTH;
  if ( ( header & OVS_LEARN_SRC_MASK ) == OVS_LEARN_SRC_IMMEDIATE ) {
    length += OVS_LEARN_SRC_IMMEDIATE_LENGTH( header );
  }
  else {
    length += ( OVS_LEARN_MATCH_LENGTH + OVS_LEARN_OFS_LENGTH );
  }
  if ( ( header & OVS_LEARN_DST_MASK ) != OVS_LEARN_DST_OUTPUT ) {
    length += ( OVS_LEARN_MATCH_LENGTH + OVS_LEARN_OFS_LENGTH );
  }
  return ( uint16_t ) length;
}


#endif // OVS_PROTOCOL_H


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
