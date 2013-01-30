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


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
