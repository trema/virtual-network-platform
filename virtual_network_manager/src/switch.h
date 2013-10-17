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


#ifndef SWITCH_H
#define SWITCH_H


#include <mysql/mysql.h>
#include "trema.h"


enum {
  SWITCH_PORT_ADDED,
  SWITCH_PORT_DELETED,
};


typedef void ( *switch_port_event_handler )(
  uint8_t event,
  uint64_t datapath_id,
  const char *name,
  uint16_t number,
  void *user_data
);


bool add_switch( uint64_t datapath_id, const char *controller_host, pid_t controller_pid );
bool delete_switch( uint64_t datapath_id, const char *controller_host, pid_t controller_pid );
bool delete_switch_by_host_pid( const char* controller_host, pid_t controller_pid );
bool switch_on_duty( uint64_t datapath_id, const char *controller_host, pid_t controller_pid );
bool add_port( uint64_t datapath_id, uint16_t port_no, const char *name );
bool delete_port( uint64_t datapath_id, uint16_t port_no, const char *name );
bool get_port_no_by_name( uint64_t datapath_id, const char *name, uint16_t *port_no );
bool set_switch_port_event_handler( switch_port_event_handler callback, void *user_data );
bool init_switch( MYSQL *db );
bool finalize_switch();


#endif // SWITCH_H


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
