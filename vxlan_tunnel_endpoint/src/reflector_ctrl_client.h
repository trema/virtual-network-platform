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


