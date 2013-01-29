/*
 * Copyright (C) 2012-2013 NEC Corporation
 * NEC Confidential
 */


#ifndef DISTRIBUTOR_H
#define DISTRIBUTOR_H


#include <netinet/in.h>
#include <stdint.h>
#include "linked_list.h"


void *distributor_main( void *args );
bool add_tunnel_endpoint( uint32_t vni, struct in_addr ip_addr, uint16_t port );
bool set_tunnel_endpoint_port( uint32_t vni, struct in_addr ip_addr, uint16_t port );
list *lookup_tunnel_endpoints( uint32_t vni );
list *get_all_tunnel_endpoints();
bool delete_tunnel_endpoint( uint32_t vni, struct in_addr ip_addr );


#endif // DISTRIBUTOR_H


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */


