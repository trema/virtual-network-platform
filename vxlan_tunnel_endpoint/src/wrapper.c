/*
 * Copyright (C) 2012-2013 NEC Corporation
 * NEC Confidential
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
