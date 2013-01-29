/*
 * Copyright (C) 2012-2013 NEC Corporation
 * NEC Confidential
 */


#include <sys/select.h>


#ifdef __FDMASK
#undef __FDMASK
#endif
#define __FDMASK( d ) ( ( __fd_mask ) 1 << ( ( d ) % __NFDBITS ) )

#define sizeof( X ) ( ( int ) sizeof( X ) )


char *safe_strerror_r( int errnum, char *buf, size_t buflen );


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
