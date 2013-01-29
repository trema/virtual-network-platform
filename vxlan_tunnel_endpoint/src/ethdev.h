/*
 * Copryright (C) 2012-2013 NEC Corporation
 * NEC Confidential
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
