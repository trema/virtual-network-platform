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


#ifndef OVS_H
#define OVS_H


#include <netinet/in.h>
#include "ovs_actions.h"
#include "ovs_byteorder.h"
#include "ovs_match.h"
#include "ovs_protocol.h"
#include "ovs_utility.h"
#include "trema.h"


#define TABLE_ID_ANY 255

#define ENABLE_FLOW_MOD_TABLE_ID 1
#define DISABLE_FLOW_MOD_TABLE_ID 0


typedef void ( *ovs_flow_removed_handler )( uint64_t datapath_id, uint32_t transaction_id,
                                            uint64_t cookie, uint16_t priority, uint8_t reason,
                                            uint32_t duration_sec, uint32_t duration_nsec,
                                            uint16_t idle_timeout, uint64_t packet_count, uint64_t byte_count,
                                            const ovs_matches *matches, void *user_data );

typedef void ( *ovs_error_handler )( uint64_t datapath_id, uint32_t transaction_id,
                                     uint16_t type, uint16_t code, const buffer *data, void *user_data );

typedef struct {
  ovs_flow_removed_handler ovs_flow_removed_callback;
  void *ovs_flow_removed_user_data;
  vendor_handler other_vendor_callback;
  void *other_vendor_user_data;
  ovs_error_handler ovs_error_callback;
  void *ovs_error_user_data;
  error_handler other_error_callback;
  void *other_error_user_data;
} ovs_event_handlers;


buffer *create_ovs_set_flow_format( const uint32_t transaction_id, const uint32_t format );
buffer *create_ovs_flow_mod( const uint32_t transaction_id, const uint64_t cookie, const uint16_t command,
                             const uint8_t table_id, const uint16_t idle_timeout, const uint16_t hard_timeout,
                             const uint16_t priority, const uint32_t buffer_id,
                             const uint16_t out_port, const uint16_t flags,
                             const ovs_matches *matches, const openflow_actions *actions );
buffer *create_ovs_flow_mod_table_id( const uint32_t transaction_id, const uint8_t set );

bool set_ovs_flow_removed_handler( ovs_flow_removed_handler callback, void *user_data );
bool set_other_vendor_handler( vendor_handler callback, void *user_data );
bool set_ovs_error_handler( ovs_error_handler callback, void *user_data );
bool set_other_error_handler( error_handler callback, void *user_data );


#endif // OVS_H


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
