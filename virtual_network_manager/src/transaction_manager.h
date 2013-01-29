/*
 * Utilities for managing OpenFlow message transactions.
 *
 * Author: Yasunobu Chiba
 *
 * Copyright (C) 2012 NEC Corporation
 * NEC Confidential
 */


#ifndef TRANSACTION_MANAGER_H
#define TRANSACTION_MANAGER_H


#include "trema.h"


typedef void ( *succeeded_handler )(
  uint64_t datapath_id,
  const buffer *original_message,
  void *user_data
);


typedef void ( *failed_handler )(
  uint64_t datapath_id,
  const buffer *original_message,
  void *user_data
);


bool execute_transaction( uint64_t datapath_id, const buffer *message,
                          succeeded_handler succeeded_callback, void *succeeded_user_data,
                          failed_handler failed_callback, void *failed_user_data );
bool init_transaction_manager();
bool finalize_transaction_manager();


#endif // TRANSACTION_MANAGER_H


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
