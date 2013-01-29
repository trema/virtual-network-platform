/*
 * Copyright (C) 2012-2013 NEC Corporation
 * NEC Confidential
 */


#ifndef REFLECTOR_CTRL_CLIENT_H
#define REFLECTOR_CTRL_CLIENT_H


#include <netinet/in.h>
#include <stdbool.h>
#include "reflector_ctrl_common.h"


bool add_tep( uint32_t vni, struct in_addr ip_addr, uint16_t port, uint8_t *reason );
bool set_tep( uint32_t vni, struct in_addr ip_addr, uint16_t set_bitmap, uint16_t port, uint8_t *reason );
bool delete_tep( uint32_t vni, struct in_addr ip_addr, uint8_t *reason );
bool list_tep( uint32_t vni, uint8_t *reason );
bool init_reflector_ctrl_client();
bool finalize_reflector_ctrl_client();


#endif // REFLECTOR_CTRL_CLINET_H


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */


