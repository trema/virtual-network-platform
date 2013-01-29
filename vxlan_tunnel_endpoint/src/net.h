/*
 * Packet forwarder implementation.
 *
 * Copryright (C) 2012 upa@haeena.net
 * Copryright (C) 2012-2013 NEC Corporation
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
