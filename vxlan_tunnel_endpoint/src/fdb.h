/*
 * Copyright (C) 2012 upa@haeena.net
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


#ifndef FDB_H
#define FDB_H


#include <pthread.h>
#include <time.h>
#include <netinet/in.h>
#include <net/ethernet.h>
#include "hash.h"
#include "linked_list.h"


enum {
  FDB_ENTRY_TYPE_DYNAMIC = 0x01,
  FDB_ENTRY_TYPE_STATIC = 0x02,
  FDB_ENTRY_TYPE_ALL = 0x03,
};


struct fdb_entry {
  uint8_t mac[ ETH_ALEN ];
  struct sockaddr_in vtep_addr;
  time_t ttl;
  struct timespec created_at;
  struct timespec deleted_at;
  uint8_t type;
};

struct fdb {
  struct hash fdb;
  time_t aging_time;
  time_t sleep_seconds;
  list *garbage;
  pthread_t decrease_ttl_t;
};


struct fdb *init_fdb( time_t aging_time );
void destroy_fdb( struct fdb *fdb );
bool fdb_add_entry( struct fdb *fdb, uint8_t *mac, struct sockaddr_in vtep_addr );
bool fdb_add_static_entry( struct fdb *fdb, struct ether_addr eth_addr, struct in_addr ip_addr, time_t aging_time );
bool fdb_delete_entry( struct fdb *fdb, struct ether_addr eth_addr );
bool fdb_delete_all_entries( struct fdb *fdb, uint8_t type );
struct fdb_entry *fdb_search_entry( struct fdb *fdb, uint8_t *mac );
bool set_aging_time( struct fdb *fdb, time_t aging_time );
list *get_fdb_entries( struct fdb *fdb );
void fdb_collect_garbage( struct fdb *fdb );


#endif // FDB_H


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
