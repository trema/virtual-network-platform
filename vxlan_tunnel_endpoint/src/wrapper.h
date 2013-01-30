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


#ifndef WRAPPER_H
#define WRAPPER_H


#include <sys/select.h>


#ifdef __FDMASK
#undef __FDMASK
#endif
#define __FDMASK( d ) ( ( __fd_mask ) 1 << ( ( d ) % __NFDBITS ) )

#define sizeof( X ) ( ( int ) sizeof( X ) )


char *safe_strerror_r( int errnum, char *buf, size_t buflen );


#endif // WRAPPER_H


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
