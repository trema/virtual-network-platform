/*
 * VXLAN protocol definition
 *
 * Copryright (C) 2012-2013 NEC Corporation
 */


#ifndef VXLAN_H
#define VXLAN_H


#include <stdbool.h>
#include <stdint.h>


#define VXLAN_DEFAULT_UDP_PORT 60000
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
