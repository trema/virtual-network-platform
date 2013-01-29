/*
 * Functions for creating/handling Open vSwitch extended actions
 *
 * Author: Yasunobu Chiba
 *
 * Copyright (C) 2012 NEC Corporation
 * NEC Confidential
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
