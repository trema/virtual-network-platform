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


#ifndef VXLAN_H
#define VXLAN_H


#include <stdbool.h>
#include <stdint.h>


#define VXLAN_DEFAULT_UDP_PORT 8472
#define VXLAN_DEFAULT_FLOODING_ADDR "239.0.0.1"
#define VXLAN_MCAST_TTL 16
#define VXLAN_DEFAULT_AGING_TIME 300
#define VXLAN_MAX_AGING_TIME 86400


#define VXLAN_VNISIZE 3
#define VXLAN_VALIDFLAG 0x08


struct vxlanhdr {
  uint8_t flags;
  uint8_t rsv1[ 3 ];
  uint8_t vni[ 3 ];
  uint8_t rsv2;
};


bool valid_vni( uint32_t vni );


#endif // VXLAN_H


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
