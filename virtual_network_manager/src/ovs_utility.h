/*
 * Utility functions for Open vSwitch extended OpenFlow protocol
 *
 * Author: Yasunobu Chiba
 *
 * Copyright (C) 2012 NEC Corporation
 * NEC Confidential
 */


#ifndef OVS_UTILITY_H
#define OVS_UTILITY_H


#include "ovs_match.h"
#include "trema.h"


bool ovs_matches_to_string( const ovs_matches *matches, char *str, size_t size );


#endif // OVS_UTILITY_H


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
