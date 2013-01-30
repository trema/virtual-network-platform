/*
 * Copyright (C) 2012 upa@haeena.net
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


#ifndef NET_H
#define NET_H


#include <net/ethernet.h>
#include <netinet/if_ether.h>
#include <netinet/in.h>
#include <stdbool.h>
#include "vxlan_common.h"
#include "vxlan_instance.h"


void send_etherframe_from_vxlan_to_local( struct vxlan_instance *instance,
                                          struct ether_header *ether, size_t len );
void send_etherframe_from_local_to_vxlan( struct vxlan_instance *instance,
                                          struct ether_header *ether, size_t len );
bool update_interface_state();
bool ipv4_multicast_join( struct in_addr addr );
bool ipv4_multicast_leave( struct in_addr addr );
bool init_net( struct vxlan *vxlan );
bool finalize_net();


#endif // NET_H


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
