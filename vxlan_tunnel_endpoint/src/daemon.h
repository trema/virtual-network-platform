/*
 * Copyright (C) 2012-2013 NEC Corporation
 * NEC Confidential
 */


#ifndef DAEMON_H
#define DAEMON_H


#include <stdbool.h>


void daemonize();
bool pid_file_exists( const char *name );
bool create_pid_file( const char *name );
bool remove_pid_file( const char *name );


#endif // DAEMON_H


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */


