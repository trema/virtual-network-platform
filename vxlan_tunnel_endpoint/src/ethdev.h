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


#ifndef ETHDEV_H
#define ETHDEV_H


#include <net/if.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdint.h>


typedef struct {
  int fd;
  int ifindex;
  char name[ IFNAMSIZ ];
  int dummy_fd;
} ethdev;


bool init_ethdev( const char *name, uint16_t port, ethdev **dev );
bool close_ethdev( ethdev *dev );
ssize_t recv_from_ethdev( ethdev *dev, char *data, size_t length, int *err );
ssize_t send_to_ethdev( ethdev *dev, const char *data, size_t length, struct sockaddr_in *addr, int *err );


#endif // ETHDEV_H


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
