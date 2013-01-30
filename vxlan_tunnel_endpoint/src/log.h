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


#ifndef LOG_H
#define LOG_H


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


#endif // LOG_H


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
