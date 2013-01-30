/*
 * Author: Yasunobu Chiba and Satoshi Tsuyama
 *
 * Copyright (C) 2012-2013 NEC Corporation
 * Copyright (C) NEC BIGLOBE, Ltd. 2012
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
