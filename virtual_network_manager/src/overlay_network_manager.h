/*
 * Overlay network manager.
 *
 * Author: Yasunobu Chiba
 *
 * Copyright (C) 2012 NEC Corporation
 * NEC Confidential
 */


#ifndef OVERLAY_NETWORK_MANAGER_H
#define OVERLAY_NETWORK_MANAGER_H


#include <mysql/mysql.h>
#include "trema.h"


enum {
  OVERLAY_NETWORK_OPERATION_FAILED = -1,
  OVERLAY_NETWORK_OPERATION_SUCCEEDED,
  OVERLAY_NETWORK_OPERATION_IN_PROGRESS,
};


typedef void ( *overlay_operation_completed_handler )(
  int status,
  void *user_data
);


int attach_to_network( uint64_t datapath_id, uint32_t vni,
                       overlay_operation_completed_handler callback, void *user_data );
int detach_from_network( uint64_t datapath_id, uint32_t vni,
                         overlay_operation_completed_handler callback, void *user_data );
bool init_overlay_network_manager( MYSQL *db );
bool finalize_overlay_network_manager();


#endif // OVERLAY_NETWORK_MANAGER_H


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
