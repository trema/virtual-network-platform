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
#include <string.h>
#include "wrapper.h"


char *
safe_strerror_r( int errnum, char *buf, size_t buflen ) {
  assert( buf != NULL );
  assert( buflen > 0 );

  int original = errnum;

  memset( buf, '\0', buflen );

#if ( _POSIX_C_SOURCE >= 200112L || _XOPEN_SOURCE >= 600 ) && ! _GNU_SOURCE
  int ret = strerror_r( errnum, buf, buflen );
  char *error_string = buf;
#else
  char *error_string = strerror_r( errnum, buf, buflen );
#endif

  errno = original;

  return error_string;
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
