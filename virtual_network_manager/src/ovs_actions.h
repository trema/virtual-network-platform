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


#ifndef OVS_ACTIONS_H
#define OVS_ACTIONS_H


#include "ovs_protocol.h"
#include "trema.h"


typedef struct {
  int n_flow_mod_specs;
  list_element *list;
} ovs_flow_mod_specs;


bool append_ovs_action_reg_load( openflow_actions *actions, const uint16_t offset, const uint8_t n_bits,
                                 const uint32_t destination, const uint64_t value );
bool append_ovs_action_note( openflow_actions *actions, const uint8_t *note, const uint16_t note_len );
bool append_ovs_action_resubmit( openflow_actions *actions, const uint16_t in_port );
bool append_ovs_action_resubmit_table( openflow_actions *actions, const uint16_t in_port, const uint8_t table_id );
bool append_ovs_action_learn( openflow_actions *actions, const uint16_t idle_timeout, const uint16_t hard_timeout,
                              const uint16_t priority, const uint64_t cookie, const uint16_t flags, const uint8_t table_id,
#if OVS_VERSION_CODE >= OVS_VERSION( 1, 6, 0 )
                              const uint16_t fin_idle_timeout, const uint16_t fin_hard_timeout,
#endif
                              const ovs_flow_mod_specs *flow_mod_specs );


ovs_flow_mod_specs *create_ovs_flow_mod_specs( void );
bool delete_ovs_flow_mod_specs( ovs_flow_mod_specs *flow_mod_specs );
bool append_ovs_flow_mod_specs_add_match_field( ovs_flow_mod_specs *flow_mod_specs, uint32_t src, uint32_t dst, uint16_t n_bits );
bool append_ovs_flow_mod_specs_add_load_field( ovs_flow_mod_specs *flow_mod_specs, uint32_t src, uint32_t dst, uint16_t n_bits );
bool append_ovs_flow_mod_specs_add_output_action( ovs_flow_mod_specs *flow_mod_specs, uint32_t src );


#endif // OVS_ACTIONS_H


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
