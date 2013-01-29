/*
 * Functions for managing database connection
 *
 * Author: Yasunobu Chiba
 *
 * Copyright (C) 2012 NEC Corporation
 * NEC Confidential
 */


#ifndef DB_H
#define DB_H


#include <limits.h>
#include <mysql/mysql.h>
#include "trema.h"


typedef struct {
  char host[ HOSTNAME_LENGTH + 1 ];
  uint16_t port;
  char username[ USERNAME_LENGTH + 1 ];
  char password[ NAME_LEN + 1 ]; // TODO: check if the length is ok or not
  char db_name[ NAME_LEN + 1 ]; // TODO: check if the length is ok or not
} db_config;


bool execute_query( MYSQL *db, const char *format, ... );
bool init_db( MYSQL **db, db_config config );
bool finalize_db( MYSQL *db );


#endif // DB_H


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
