/*
 * Logging functions.
 *
 * Copryright (C) 2012-2013 NEC Corporation
 * NEC Confidential
 */


#include <stdint.h>
#include <stdio.h>
#include <syslog.h>


#define LOG_OUTPUT_STDOUT 0x01
#define LOG_OUTPUT_SYSLOG 0x02

#define critical( ... ) do_log( LOG_CRIT, __VA_ARGS__ );
#define error( ... ) do_log( LOG_ERR, __VA_ARGS__ );
#define warn( ... ) do_log( LOG_WARNING, __VA_ARGS__ );
#define notice( ... ) do_log( LOG_NOTICE, __VA_ARGS__ );
#define info( ... ) do_log( LOG_INFO, __VA_ARGS__ );
#define debug( ... ) do_log( LOG_DEBUG, __VA_ARGS__ );


void set_log_level( int priority );
void do_log( int priority, const char *format, ... );
void init_log( const char *ident, uint8_t output );
void finalize_log();


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
