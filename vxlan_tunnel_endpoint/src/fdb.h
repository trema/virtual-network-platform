/*
 * Forwarding database implementation.
 *
 * Copryright (C) 2012 upa@haeena.net
 * Copryright (C) 2012-2013 NEC Corporation
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
  uint8_t type;
};

struct fdb {
  struct hash fdb;
  time_t aging_time;
  time_t sleep_seconds;
  pthread_t decrease_ttl_t;
};


struct fdb *init_fdb( time_t aging_time );
void destroy_fdb( struct fdb *fdb );
bool fdb_add_entry( struct fdb *fdb, uint8_t *mac, struct sockaddr_in vtep_addr );
bool fdb_add_static_entry( struct fdb *fdb, struct ether_addr eth_addr, struct in_addr ip_addr, time_t aging_time );
bool fdb_delete_entry( struct fdb *fdb, struct ether_addr eth_addr, uint8_t type );
bool fdb_delete_all_entries( struct fdb *fdb, uint8_t type );
struct fdb_entry *fdb_search_entry( struct fdb *fdb, uint8_t *mac );
bool set_aging_time( struct fdb *fdb, time_t aging_time );
list *get_fdb_entries( struct fdb *fdb );


#endif // FDB_H


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
