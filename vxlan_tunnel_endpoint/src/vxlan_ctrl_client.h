/*
 * Copyright (C) 2012-2013 NEC Corporation
 * NEC Confidential
 */


#ifndef VXLAN_CTRL_CLIENT_H
#define VXLAN_CTRL_CLIENT_H


#include <stdbool.h>
#include <time.h>
#include "vxlan_ctrl_common.h"


bool add_instance( uint32_t vni, struct in_addr flooding_addr, uint16_t port, time_t aging_time, uint8_t *reason );
bool set_instance( uint32_t vni, uint16_t set_bitmap, struct in_addr flooding_addr, uint16_t port, time_t aging_time,
                   uint8_t *reason );
bool inactivate_instance( uint32_t vni, uint8_t *reason );
bool activate_instance( uint32_t vni, uint8_t *reason );
bool delete_instance( uint32_t vni, uint8_t *reason );
bool show_global( uint16_t set_bitmap, uint8_t *reason );
bool list_instances( uint32_t vni, uint16_t set_bitmap, uint8_t *reason );
bool show_fdb( uint32_t vni, uint8_t *reason );
bool add_fdb_entry( uint32_t vni, struct ether_addr eth_addr, struct in_addr ip_addr, time_t aging_time,
                    uint8_t *reason );
bool delete_fdb_entry( uint32_t vni, struct ether_addr eth_addr, uint8_t *reason );
bool init_vxlan_ctrl_client();
bool finalize_vxlan_ctrl_client();


#endif // VXLAN_CTRL_CLINET_H


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */


