/*
 * Copyright (C) 2012-2013 NEC Corporation
 * NEC Confidential
 */


#ifndef VXLAN_CTRL_SERVER_H
#define VXLAN_CTRL_SERVER_H


#include <stdbool.h>
#include "vxlan_ctrl_common.h"


bool start_vxlan_ctrl_server();
bool init_vxlan_ctrl_server();
bool finalize_vxlan_ctrl_server();


#endif // VXLAN_CTRL_SERVER_H


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */


