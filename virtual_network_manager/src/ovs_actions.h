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


bool append_ovs_action_reg_load( openflow_actions *actions, const uint16_t offset, const uint8_t n_bits,
                                 const uint32_t destination, const uint64_t value );
bool append_ovs_action_resubmit( openflow_actions *actions, const uint16_t in_port );
bool append_ovs_action_resubmit_table( openflow_actions *actions, const uint16_t in_port, const uint8_t table_id );


#endif // OVS_ACTIONS_H


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
