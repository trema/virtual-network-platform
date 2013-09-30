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
#include "ovs_actions.h"
#include "trema.h"


bool
append_ovs_action_reg_load( openflow_actions *actions, const uint16_t offset, const uint8_t n_bits,
                            const uint32_t destination, const uint64_t value ) {
  assert( actions != NULL );

  ovs_action_reg_load *action_reg_load = xmalloc( sizeof( ovs_action_reg_load ) );
  memset( action_reg_load, 0, sizeof( ovs_action_reg_load ) );

  action_reg_load->type = OFPAT_VENDOR;
  action_reg_load->len = ( uint16_t ) sizeof( ovs_action_reg_load );
  action_reg_load->vendor = OVS_VENDOR_ID;
  action_reg_load->subtype = OVSAST_REG_LOAD;
  action_reg_load->ofs_nbits = ( uint16_t ) ( ( offset << 6 ) | ( n_bits - 1 ) );
  action_reg_load->dst = destination;
  action_reg_load->value = value;

  bool ret = append_to_tail( &actions->list, action_reg_load );
  if ( ret ) {
    actions->n_actions++;
  }

  return ret;
}


static bool
append_ovs_action_resubmit_with_type( openflow_actions *actions, const uint16_t type,
                                      const uint16_t in_port, const uint8_t table_id ) {
  assert( actions != NULL );
  assert( type == OVSAST_RESUBMIT || type == OVSAST_RESUBMIT_TABLE );

  ovs_action_resubmit *action_resubmit = xmalloc( sizeof( ovs_action_resubmit ) );
  memset( action_resubmit, 0, sizeof( ovs_action_resubmit ) );

  action_resubmit->type = OFPAT_VENDOR;
  action_resubmit->len = ( uint16_t ) sizeof( ovs_action_resubmit );
  action_resubmit->vendor = OVS_VENDOR_ID;
  action_resubmit->subtype = type;
  action_resubmit->in_port = in_port;
  if ( type == OVSAST_RESUBMIT_TABLE ) {
    action_resubmit->table = table_id;
  }
  else {
    action_resubmit->table = 0;
  }

  bool ret = append_to_tail( &actions->list, action_resubmit );
  if ( ret ) {
    actions->n_actions++;
  }

  return ret;
}


bool
append_ovs_action_resubmit( openflow_actions *actions, const uint16_t in_port ) {
  return append_ovs_action_resubmit_with_type( actions, OVSAST_RESUBMIT, in_port, 0 );
}


bool
append_ovs_action_resubmit_table( openflow_actions *actions,
                                  const uint16_t in_port, const uint8_t table_id ) {
  return append_ovs_action_resubmit_with_type( actions, OVSAST_RESUBMIT_TABLE, in_port, table_id );
}


bool
append_ovs_action_learn( openflow_actions *actions, const uint16_t idle_timeout, const uint16_t hard_timeout,
                         const uint16_t priority, const uint64_t cookie, const uint16_t flags, const uint8_t table_id,
                         const ovs_flow_mod_specs *flow_mod_specs ) {
  assert( actions != NULL );
  assert( flow_mod_specs != NULL );

  size_t flow_mod_specs_len = 0;
  list_element *element = flow_mod_specs->list;
  while ( element != NULL ) {
    uint16_t *header = element->data;
    flow_mod_specs_len += ovs_flow_mod_spec_length( *header );
    element = element->next;
  }
  size_t len = ( sizeof( ovs_action_learn ) + flow_mod_specs_len ) % 8;
  if ( len > 0 ) {
    flow_mod_specs_len += ( 8 - len );
  }
  ovs_action_learn *action_learn = xmalloc( sizeof( ovs_action_learn ) + flow_mod_specs_len );
  memset( action_learn, 0, sizeof( ovs_action_learn ) + flow_mod_specs_len );

  action_learn->type = OFPAT_VENDOR;
  action_learn->len = ( uint16_t ) ( sizeof( ovs_action_learn ) + flow_mod_specs_len );
  action_learn->vendor = OVS_VENDOR_ID;
  action_learn->subtype = OVSAST_LEARN;
  action_learn->idle_timeout = idle_timeout;
  action_learn->hard_timeout = hard_timeout;
  action_learn->priority = priority;
  action_learn->cookie = cookie;
  action_learn->flags = flags;
  action_learn->table_id = table_id;
  char *p = ( char * ) action_learn + sizeof( ovs_action_learn );
  element = flow_mod_specs->list;
  while ( element != NULL ) {
    uint16_t *header = element->data;
    size_t spec_len = ovs_flow_mod_spec_length( *header );
    memcpy( p, header, spec_len );
    p += spec_len;
    element = element->next;
  }

  bool ret = append_to_tail( &actions->list, action_learn );
  if ( ret ) {
    actions->n_actions++;
  }

  return ret;
}


ovs_flow_mod_specs *
create_ovs_flow_mod_specs() {
  ovs_flow_mod_specs *flow_mod_specs = xmalloc( sizeof( ovs_flow_mod_specs ) );

  if ( create_list( &flow_mod_specs->list ) == false ) {
    assert( 0 );
  }

  flow_mod_specs->n_flow_mod_specs = 0;

  return flow_mod_specs;
}


bool
delete_ovs_flow_mod_specs( ovs_flow_mod_specs *flow_mod_specs ) {
  assert( flow_mod_specs != NULL );

  list_element *element = flow_mod_specs->list;
  while ( element != NULL ) {
    xfree( element->data );
    element = element->next;
  }

  delete_list( flow_mod_specs->list );
  xfree( flow_mod_specs );

  return true;
}


static bool
append_ovs_flow_mod_specs( ovs_flow_mod_specs *flow_mod_specs, uint16_t header, void *src, uint16_t src_ofs, uint32_t dst, uint16_t dst_ofs  ) {
  assert( flow_mod_specs != NULL );
  assert( src != NULL );

  size_t spec_len = ovs_flow_mod_spec_length( header );
  char *flow_mod_spec = xmalloc( spec_len );
  memset( flow_mod_spec, 0, spec_len );

  char *p = flow_mod_spec;
  memcpy( p, &header, OVS_LEARN_HEADER_LENGTH );
  p += OVS_LEARN_HEADER_LENGTH;
  if ( ( header & OVS_LEARN_SRC_MASK ) == OVS_LEARN_SRC_IMMEDIATE ) {
    int src_len = OVS_LEARN_SRC_IMMEDIATE_LENGTH( header );
    memcpy( p, src, ( size_t ) src_len );
    p += src_len;
  }
  else {
    memcpy( p, src, OVS_LEARN_MATCH_LENGTH );
    p += OVS_LEARN_MATCH_LENGTH;
    memcpy( p, &src_ofs, OVS_LEARN_OFS_LENGTH );
    p += OVS_LEARN_OFS_LENGTH;
  }
  if ( ( header & OVS_LEARN_DST_MASK ) != OVS_LEARN_DST_OUTPUT ) {
    memcpy( p, &dst, OVS_LEARN_MATCH_LENGTH );
    p += OVS_LEARN_MATCH_LENGTH;
    memcpy( p, &dst_ofs, OVS_LEARN_OFS_LENGTH );
    p += OVS_LEARN_OFS_LENGTH;
  }

  bool ret = append_to_tail( &flow_mod_specs->list, ( void * ) flow_mod_spec );
  if ( ret == true ) {
    flow_mod_specs->n_flow_mod_specs++;
  }

  return ret;
}


bool
append_ovs_flow_mod_specs_add_match_field( ovs_flow_mod_specs *flow_mod_specs, uint32_t src, uint32_t dst, uint16_t n_bits ) {
  assert( flow_mod_specs != NULL );

  uint16_t header = ( uint16_t ) ( OVS_LEARN_SRC_FIELD | OVS_LEARN_DST_MATCH | ( n_bits & OVS_LEARN_N_BITS_MASK ) );
  uint16_t src_ofs = 0;
  uint16_t dst_ofs = 0;

  return append_ovs_flow_mod_specs( flow_mod_specs, header, &src, src_ofs, dst, dst_ofs );
}


bool
append_ovs_flow_mod_specs_add_load_field( ovs_flow_mod_specs *flow_mod_specs, uint32_t src, uint32_t dst, uint16_t n_bits ) {
  assert( flow_mod_specs != NULL );

  uint16_t header = ( uint16_t ) ( OVS_LEARN_SRC_FIELD | OVS_LEARN_DST_LOAD | ( n_bits & OVS_LEARN_N_BITS_MASK ) );
  uint16_t src_ofs = 0;
  uint16_t dst_ofs = 0;

  return append_ovs_flow_mod_specs( flow_mod_specs, header, &src, src_ofs, dst, dst_ofs );
}


bool
append_ovs_flow_mod_specs_add_output_action( ovs_flow_mod_specs *flow_mod_specs, uint32_t src ) {
  assert( flow_mod_specs != NULL );

  uint16_t n_bits = 16;
  uint16_t header = ( uint16_t ) ( OVS_LEARN_SRC_FIELD | OVS_LEARN_DST_OUTPUT | ( n_bits & OVS_LEARN_N_BITS_MASK ) );
  uint16_t src_ofs = 0;

  return append_ovs_flow_mod_specs( flow_mod_specs, header, &src, src_ofs, 0, 0 );
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
