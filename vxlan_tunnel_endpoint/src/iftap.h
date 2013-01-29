/*
 * Functions for managing TAP interfaces.
 *
 * Copryright (C) 2012 upa@haeena.net
 * Copryright (C) 2012-2013 NEC Corporation
 */


#ifndef IFTAP_H
#define IFTAP_H


#include <stdbool.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include <linux/if_tun.h>


int tap_alloc( const char *dev );
bool tap_up( const char *dev );
bool tap_down( const char *dev );


#endif // IFTAP_H


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
