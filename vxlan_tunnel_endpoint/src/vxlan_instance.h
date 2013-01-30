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


#ifndef VXLAN_INSTANCE_H
#define VXLAN_INSTANCE_H


#include <net/ethernet.h>
#include <netinet/in.h>
#include "vxlan_common.h"
#include "vxlan.h"


struct vxlan_instance {
  uint8_t vni[ VXLAN_VNISIZE ];
  struct sockaddr_in addr;
  uint16_t port;
  int udp_sock;
  char vxlan_tap_name[ IFNAMSIZ ];
  bool multicast_joined;
  struct fdb *fdb;
  pthread_t tid;
  int tap_sock;
  time_t aging_time;
  bool activated;
};


struct vxlan_instance *create_vxlan_instance( uint8_t *vni, struct in_addr addr, uint16_t port, time_t aging_time );
struct vxlan_instance **get_all_vxlan_instances( int *n_instances );
bool start_vxlan_instance( struct vxlan_instance *vins );
bool set_vxlan_instance_flooding_addr( uint8_t *vni, struct in_addr addr );
bool set_vxlan_instance_port( uint8_t *vni, uint16_t port );
bool set_vxlan_instance_aging_time( uint8_t *vni, time_t aging_time );
bool inactivate_vxlan_instance( uint8_t *vni );
bool activate_vxlan_instance( uint8_t *vni );
bool destroy_vxlan_instance( struct vxlan_instance *vins );
void process_fdb_etherframe_from_vxlan( struct vxlan_instance *vins,
                                        struct ether_header *ether,
                                        struct sockaddr_in *vtep_addr );
bool init_vxlan_instances( struct vxlan *vxlan );
bool finalize_vxlan_instances();


#endif // VXLAN_INSTANCE_H


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
