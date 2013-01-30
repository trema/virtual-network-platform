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


#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <linux/limits.h>
#include <mysql/mysql.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "db.h"
#ifdef UNIT_TEST
#include "db_unittest.h"
#endif


bool
execute_query( MYSQL *db, const char *format, ... ) {
  assert( db != NULL );
  assert( format != NULL );

  debug( "Checking if a database connection is alive or not ( db = %p ).", db );
  int ret = mysql_ping( db );
  if ( ret != 0 ) {
    info( "Database connection has been lost ( ret = %d ). Reconnecting.", ret );
  }

  char *statement = NULL;
  va_list args;
  va_start( args, format );
  ret = vasprintf( &statement, format, args );
  va_end( args );
  if ( ret < 0 ) {
    error( "Failed to format a SQL statement ( ret = %d ).", ret );
    return false;
  }
  assert( statement != NULL );

  long unsigned int length = ( long unsigned int ) strlen( statement );

  debug( "Executing a SQL statement ( statement = '%s', length = %lu ).", statement, length );

  ret = mysql_real_query( db, statement, length );
  if ( ret != 0 ) {
    error( "Failed to execute a SQL statement ( statement = '%s', ret = %d, error = '%s' ).", statement, ret, mysql_error( db ) );    
    xfree( statement );
    return false;
  }

  debug( "A SQL statement is executed successfully ( statement = '%s', length = %lu ).", statement, length );

  xfree( statement );

  return true;
}


bool
init_db( MYSQL **db, db_config config ) {
  assert( db != NULL );
  assert( *db == NULL );

  debug( "Initializing backend database ( db = %p, host = %s, port = %u, username = %s, password = %s, db_name = %s ).",
         db, config.host, config.port, config.username, config.password, config.db_name );

  *db = mysql_init( NULL );
  if ( *db == NULL ) {
    error( "Failed to initialize backend database." );
    return false;
  }

  my_bool reconnect = 1;
  int ret = mysql_options( *db, MYSQL_OPT_RECONNECT, &reconnect );
  if ( ret != 0 ) {
    error( "Failed to enable automatic reconnect option ( db = %p, error = '%s' ).", db, mysql_error( *db ) );
    mysql_close( *db );    
    return false;
  }

  MYSQL *handle = mysql_real_connect( *db, config.host, config.username, config.password,
                                      config.db_name, config.port, NULL, 0 );
  if ( handle == NULL ) {
    error( "Failed to connect to database ( db = %p, error = '%s' ).", db, mysql_error( *db ) );
    mysql_close( *db );
    return false;
  }

  debug( "Initialization completed successfully ( db = %p ).", db );

  return true;
}


bool
finalize_db( MYSQL *db ) {
  debug( "Finalizing backend database ( db = %p ).", db );

  if ( db == NULL ) {
    warn( "Backend database is already closed or not initialized yet ( db = %p ).", db );
    return false;
  }

  mysql_close( db );

  debug( "Finalization completed successfully ( db = %p ).", db );

  return true;
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
