/*
 * Author: Yasunobu Chiba
 *
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
