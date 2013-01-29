/*
 * Copyright (C) 2012-2013 NEC Corporation
 * NEC Confidential
 */


#ifndef RECEIVER_H
#define RECEIVER_H


#include <stdint.h>
#include "ethdev.h"


typedef struct {
  ethdev *dev;
  uint16_t port;
} receiver_options;


void *receiver_main( void *args );


#endif // RECEIVER_H


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */


