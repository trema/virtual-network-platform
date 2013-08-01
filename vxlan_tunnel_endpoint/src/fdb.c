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


#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#include "fdb.h"
#include "log.h"
#include "vxlan_common.h"
#include "wrapper.h"


#define SUB_TIMESPEC( _a, _b, _return )                       \
  do {                                                        \
    ( _return )->tv_sec = ( _a )->tv_sec - ( _b )->tv_sec;    \
    ( _return )->tv_nsec = ( _a )->tv_nsec - ( _b )->tv_nsec; \
    if ( ( _return )->tv_nsec < 0 ) {                         \
      ( _return )->tv_sec--;                                  \
      ( _return )->tv_nsec += 1000000000;                     \
    }                                                         \
  }                                                           \
  while ( 0 )


static bool
now( struct timespec *ts ) {
  assert( ts != NULL );

  char buf[ 256 ];

  int ret = clock_gettime( CLOCK_MONOTONIC, ts );
  if ( ret < 0 ) {
    const char *error_string = safe_strerror_r( errno, buf, sizeof( buf ) );
    error( "Failed to add a forwarding database entry ( %s [%d] ).", error_string, errno );
    return false;
  }

  return true;
}


static void
append_to_garbage_list( struct fdb *fdb, struct fdb_entry *entry ) {
  assert( fdb != NULL );
  assert( fdb->garbage != NULL );
  assert( entry != NULL );

  now( &entry->deleted_at );
  append_to_tail( fdb->garbage, entry );
}


void *
fdb_decrease_ttl_thread( void *param ) {
  assert( param != NULL );

  struct fdb *fdb = ( struct fdb * ) param;
  struct hash *hash = &fdb->fdb;

  while ( running ) {
    for ( int n = 0; n < HASH_TABLE_SIZE; n++ ) {
      pthread_mutex_lock( &hash->mutex[ n ] );
      struct hashnode *prev = &hash->table[ n ];
      for ( struct hashnode *ptr = hash->table[ n ].next; ptr != NULL; ptr = ptr->next ) {
        struct fdb_entry *entry = ( struct fdb_entry * ) ptr->data;
        if ( entry->ttl >= 0 ) {
          entry->ttl -= fdb->sleep_seconds;
          if ( entry->ttl <= 0 ) {
            prev->next = ptr->next;
            free( ptr->key );
            free( ptr );
            append_to_garbage_list( fdb, entry );
            ptr = prev;
            fdb->fdb.count--;
          }
          else {
            prev = ptr;
          }
        }
        else {
          prev = ptr;
        }
      }
      pthread_mutex_unlock( &hash->mutex[ n ] );
    }
    // FIXME: use timerfd instead of sleep
    sleep( ( unsigned int ) fdb->sleep_seconds );
  }

  return NULL;
}


static void
fdb_decrease_ttl_thread_init( struct fdb *fdb ) {
  assert( fdb != NULL );

  pthread_attr_t attr;

  pthread_attr_init( &attr );
  int ret = pthread_attr_setstacksize( &attr, 128 * 1024 );
  if ( ret != 0 ) {
    critical( "Failed to set stack size for a FDB management thread." );
  }
  ret = pthread_create( &fdb->decrease_ttl_t, &attr, fdb_decrease_ttl_thread, fdb );
  if ( ret != 0 ) {
    critical( "Failed to create a FDB management thread." );
  }
}


static void
set_sleep_seconds( struct fdb *fdb ) {
  assert( fdb != NULL );

  fdb->sleep_seconds = fdb->aging_time / 10;
  if ( fdb->sleep_seconds <= 0 ) {
    fdb->sleep_seconds = 1;
  }
  else if ( fdb->sleep_seconds > 10 ) {
    fdb->sleep_seconds = 10;
  }
}


struct fdb *
init_fdb( time_t aging_time ) {
  struct fdb *fdb = ( struct fdb * ) malloc( sizeof( struct fdb ) );
  init_hash( &fdb->fdb, 6 );
  fdb->garbage = create_list();
  fdb->aging_time = aging_time;
  if ( aging_time > 0 ) {
    set_sleep_seconds( fdb );
    fdb_decrease_ttl_thread_init( fdb );
  }

  return fdb;
}


static void
terminate_decrease_ttl_thread( struct fdb *fdb ) {
  assert( fdb != NULL );

  void *retval = NULL;
  if ( pthread_tryjoin_np( fdb->decrease_ttl_t, &retval ) == EBUSY ) {
    pthread_cancel( fdb->decrease_ttl_t );
    while ( pthread_tryjoin_np( fdb->decrease_ttl_t, &retval ) == EBUSY ) {
      struct timespec req = { 0, 50000000 };
      nanosleep( &req, NULL );
    }
  }
}


void
destroy_fdb( struct fdb *fdb ) {
  assert( fdb != NULL );

  if ( fdb->aging_time > 0 ) {
    terminate_decrease_ttl_thread( fdb );
  }

  destroy_hash( &fdb->fdb );
  delete_list_totally( fdb->garbage );
}


bool
fdb_add_entry( struct fdb *fdb, uint8_t *mac, struct sockaddr_in vtep_addr ) {
  assert( fdb != NULL );
  assert( mac != NULL );

  struct fdb_entry *entry = malloc( sizeof( struct fdb_entry ) );
  memset( entry, 0, sizeof( struct fdb_entry ) );

  entry->vtep_addr = vtep_addr;
  if ( fdb->aging_time > 0 ) {
    entry->ttl = fdb->aging_time;
  }
  else {
    entry->ttl = -1;
  }
  memcpy( entry->mac, mac, ETH_ALEN );

  now( &entry->created_at );

  entry->type = FDB_ENTRY_TYPE_DYNAMIC;

  int ret = insert_hash( &fdb->fdb, entry, mac );
  return ( ret == 1 ) ? true : false;
}


bool
fdb_add_static_entry( struct fdb *fdb, struct ether_addr eth_addr, struct in_addr ip_addr, time_t aging_time ) {
  assert( fdb != NULL );

  fdb_delete_entry( fdb, eth_addr );

  struct fdb_entry *entry = malloc( sizeof( struct fdb_entry ) );
  memset( entry, 0, sizeof( struct fdb_entry ) );

  entry->vtep_addr.sin_addr = ip_addr;
  if ( aging_time > 0 ) {
    entry->ttl = aging_time;
  }
  else {
    entry->ttl = -1;
  }
  memcpy( entry->mac, eth_addr.ether_addr_octet, ETH_ALEN );

  now( &entry->created_at );

  entry->type = FDB_ENTRY_TYPE_STATIC;

  int ret = insert_hash( &fdb->fdb, entry, eth_addr.ether_addr_octet );
  if ( ret != 1 ) {
    free( entry );
    return false;
  }

  return true;
}


bool
fdb_delete_entry( struct fdb *fdb, struct ether_addr eth_addr ) {
  assert( fdb != NULL );

  struct fdb_entry *deleted = delete_hash( &fdb->fdb, eth_addr.ether_addr_octet );
  if ( deleted != NULL ) {
    append_to_garbage_list( fdb, deleted );
  }

  return ( deleted != NULL ) ? true : false;
}


bool
fdb_delete_all_entries( struct fdb *fdb, uint8_t type ) {
  assert( fdb != NULL );

  struct hash *hash = &fdb->fdb;
  for ( int n = 0; n < HASH_TABLE_SIZE; n++ ) {
    pthread_mutex_lock( &hash->mutex[ n ] );
    struct hashnode *prev = &hash->table[ n ];
    for ( struct hashnode *ptr = hash->table[ n ].next; ptr != NULL; ptr = ptr->next ) {
      struct fdb_entry *entry = ( struct fdb_entry * ) ptr->data;
      if ( entry->type & type ) {
        prev->next = ptr->next;
        free( ptr->key );
        free( ptr );
        append_to_garbage_list( fdb, entry );
        ptr = prev;
        fdb->fdb.count--;
      }
      else {
        prev = ptr;
      }
    }
    pthread_mutex_unlock( &hash->mutex[ n ] );
  }

  return true;
}


struct fdb_entry *
fdb_search_entry( struct fdb *fdb, uint8_t *mac ) {
  assert( fdb != NULL );
  assert( mac != NULL );

  return search_hash( &fdb->fdb, mac );
}


bool
set_aging_time( struct fdb *fdb, time_t aging_time ) {
  assert( fdb != NULL );

  if ( fdb->aging_time <= 0 && aging_time > 0 ) {
    fdb->aging_time = aging_time;
    set_sleep_seconds( fdb );
    fdb_decrease_ttl_thread_init( fdb );
  }
  else if ( fdb->aging_time > 0 && aging_time > 0 ) {
    time_t old_aging_time = fdb->aging_time;
    if ( old_aging_time > aging_time ) {
      time_t diff = old_aging_time - aging_time;
      struct hash *hash = &fdb->fdb;
      for ( int n = 0; n < HASH_TABLE_SIZE; n++ ) {
        pthread_mutex_lock( &hash->mutex[ n ] );
        struct hashnode *prev = &hash->table[ n ];
        for ( struct hashnode *ptr = hash->table[ n ].next; ptr != NULL; ptr = ptr->next ) {
          struct fdb_entry *entry = ( struct fdb_entry * ) ptr->data;
          entry->ttl -= diff;
          if ( entry->ttl <= 0 ) {
            prev->next = ptr->next;
            free( ptr->key );
            free( ptr );
            append_to_garbage_list( fdb, entry );
            ptr = prev;
            fdb->fdb.count--;
          }
          else {
            prev = ptr;
          }
        }
        pthread_mutex_unlock( &hash->mutex[ n ] );
      }
    }
    else if ( old_aging_time < aging_time ) {
      time_t diff = aging_time - old_aging_time;
      struct hash *hash = &fdb->fdb;
      for ( int n = 0; n < HASH_TABLE_SIZE; n++ ) {
        pthread_mutex_lock( &hash->mutex[ n ] );
        for ( struct hashnode *ptr = hash->table[ n ].next; ptr != NULL; ptr = ptr->next ) {
          struct fdb_entry *entry = ( struct fdb_entry * ) ptr->data;
          entry->ttl += diff;
        }
        pthread_mutex_unlock( &hash->mutex[ n ] );
      }
    }
    fdb->aging_time = aging_time;
    set_sleep_seconds( fdb );
  }
  else if ( fdb->aging_time > 0 && aging_time <= 0 ) {
    fdb->aging_time = 0;
    terminate_decrease_ttl_thread( fdb );
  }
  else {
    return false;
  }

  return true;
}


list *
get_fdb_entries( struct fdb *fdb ) {
  assert( fdb != NULL );

  list *entries = create_list();
  assert( entries != NULL );
  int n_entries = 0;
  struct hash *hash = &fdb->fdb;
  for ( int n = 0; n < HASH_TABLE_SIZE; n++ ) {
    pthread_mutex_lock( &hash->mutex[ n ] );
    for ( struct hashnode *ptr = hash->table[ n ].next; ptr != NULL; ptr = ptr->next ) {
      struct fdb_entry *entry = malloc( sizeof( struct fdb_entry ) );
      assert( entry != NULL );
      memcpy( entry, ptr->data, sizeof( struct fdb_entry ) );
      append_to_tail( entries, entry );
      n_entries++;
    }
    pthread_mutex_unlock( &hash->mutex[ n ] );
  }

  if ( n_entries == 0 ) {
    delete_list( entries );
    return NULL;
  }

  return entries;
}


void
fdb_collect_garbage( struct fdb *fdb ) {
  assert( fdb != NULL );
  assert( fdb->garbage != NULL );

  if ( fdb->garbage->head == NULL ) {
    return;
  }

  pthread_mutex_lock( &fdb->garbage->mutex );

  struct timespec diff = { 0, 0 };
  now( &diff );

  for ( int i = 0; i < 256 && fdb->garbage->head != NULL; i++ ) {
    struct fdb_entry *entry = fdb->garbage->head->data;
    SUB_TIMESPEC( &diff, &entry->deleted_at, &diff );
    if ( diff.tv_sec < 2 ) {
      break;
    }
    list_element *delete_me = fdb->garbage->head;
    fdb->garbage->head = delete_me->next;
    free( delete_me->data );
    free( delete_me );
  }

  pthread_mutex_unlock( &fdb->garbage->mutex );
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
