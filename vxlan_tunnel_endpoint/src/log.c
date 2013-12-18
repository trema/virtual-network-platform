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
#include <linux/limits.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include "checks.h"
#include "log.h"


static const char *log_levels[] = { "critical", "error", "warning", "notice", "info", "debug" };

static pthread_mutex_t mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
static uint8_t output_to = 0;
static char ident_string[ PATH_MAX ];
static int log_level = LOG_INFO;


void
set_log_level( int priority ) {
  assert( priority >= LOG_CRIT );
  assert( priority <= LOG_DEBUG );

  log_level = priority;
}


static void
log_stdout( const char *format, va_list ap ) {
  size_t length = strlen( format ) + 2;
  char format_newline[ length ];
  snprintf( format_newline, length, "%s\n", format );
  vprintf( format_newline, ap );
  fflush( stdout );
}


void
do_log( int priority, const char *format, ... ) {
  assert( priority >= LOG_CRIT );
  assert( priority <= LOG_DEBUG );
  assert( format != NULL );

  if ( priority > log_level ) {
    return;
  }

  pthread_mutex_lock( &mutex );

  va_list args;
  if ( output_to & LOG_OUTPUT_STDOUT ) {
    va_start( args, format );
    log_stdout( format, args );
    va_end( args );
  }
  if ( output_to & LOG_OUTPUT_SYSLOG ) {
    va_start( args, format );
    vsyslog( priority, format, args );
    va_end( args );
  }

  pthread_mutex_unlock( &mutex );
}


static void
set_log_level_from_environment_variable() {
  const char *string = getenv( "LOG_LEVEL" );
  if ( string == NULL ) {
    return;
  }

  for ( int i = 0; i <= ( LOG_DEBUG - LOG_CRIT ); i++ ) {
    if ( strncasecmp( string, log_levels[ i ], strlen( log_levels[ i ] ) ) == 0 ) {
      set_log_level( i + LOG_CRIT );
    }
  }
}


void
init_log( const char *ident, uint8_t output ) {
  assert( ident != NULL );

  set_log_level_from_environment_variable();

  output_to = output;

  if ( output_to & LOG_OUTPUT_SYSLOG ) {
    memset( ident_string, '\0', sizeof( ident_string ) );
    strncpy( ident_string, ident, sizeof( ident_string ) - 1 );
    openlog( ident_string, LOG_NDELAY, LOG_USER );
  }
}


void
finalize_log() {
  if ( output_to & LOG_OUTPUT_SYSLOG ) {
    closelog();
    memset( ident_string, '\0', sizeof( ident_string ) );
  }

  output_to = 0;
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
