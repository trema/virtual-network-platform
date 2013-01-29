/*
 * Copryright (C) 2012 NEC Corporation
 */


#include "vxlan.h"


static const uint32_t VXLAN_VNI_MASK = 0x00ffffff;


bool
valid_vni( uint32_t vni ) {
  if ( ( vni & ~VXLAN_VNI_MASK ) != 0 ) {
    return false;
  }

  return true;
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
