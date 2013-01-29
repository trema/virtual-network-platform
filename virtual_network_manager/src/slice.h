/*
 * Slicing functions for creating virtual layer 2 domains.
 *
 * Author: Yasunobu Chiba
 *
 * Copyright (C) 2012 NEC Corporation
 * NEC Confidential
 */


#ifndef SLICE_H
#define SLICE_H


#include <mysql/mysql.h>
#include "trema.h"


bool init_slice( MYSQL *db );
bool finalize_slice();


#endif // SLICE_H


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
