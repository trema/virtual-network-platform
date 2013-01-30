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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "hash.h"


void
init_hash( struct hash *hash, int keylen ) {
  assert( hash != NULL );

  memset( hash, 0, sizeof( struct hash ) );
  for ( int n = 0; n < HASH_TABLE_SIZE; n++ ) {
    hash->table[ n ].data = NULL;
    hash->table[ n ].next = NULL;
    pthread_mutex_init( &hash->mutex[ n ], NULL );
  }

  hash->keylen = keylen;
  hash->count = 0;
}


int
calculate_hash( void *key, int keylen ) {
  assert( key != NULL );
  assert( keylen > 0 );

  if ( key == NULL ) {
    return -1;
  }

  uint32_t sum = 0;
  uint8_t *buf = ( uint8_t * ) key;
  for ( int len = keylen; len > 0; len-- ) {
    sum += *buf;
    buf++;
  }

  return sum % HASH_TABLE_SIZE;
}


int
compare_key( void *key1, void *key2, int keylen ) {
  assert( key1 != NULL );
  assert( key2 != NULL );
  assert( keylen > 0 );

  uint8_t *kb1 = ( uint8_t * ) key1;
  uint8_t *kb2 = ( uint8_t * ) key2;

  if ( ( key1 == NULL ) || ( key2 == NULL ) ) {
    return -1;
  }

  for ( int i = 0; i < keylen; i++ ) {
    if ( *kb1 != *kb2 ) {
      return -1;
    }
    kb1++;
    kb2++;
  }

  return 0;
}


int
insert_hash( struct hash *hash, void *data, void *key ) {
  assert( hash != NULL );
  assert( data != NULL );
  assert( key != NULL );

  int hash_value = -1;
  if ( ( hash_value = calculate_hash( key, hash->keylen ) ) < 0 ) {
    return -1;
  }

  struct hashnode *prev = &hash->table[ hash_value ];
  pthread_mutex_lock( &hash->mutex[ hash_value ] );

  for ( struct hashnode *ptr = prev->next; ptr != NULL; ptr = ptr->next ) {
    if ( compare_key( key, ptr->key, hash->keylen ) == 0 ) {
      pthread_mutex_unlock( &hash->mutex[ hash_value ] );
      return -1;
    }
    prev = ptr;
  }

  struct hashnode *node = ( struct hashnode * ) malloc( sizeof( struct hashnode ) );
  memset( node, 0, sizeof( struct hashnode ) );
  node->next = NULL;
  node->data = data;
  node->key = ( uint8_t * ) malloc( ( size_t ) hash->keylen );
  memcpy( node->key, key, ( size_t ) hash->keylen );
  prev->next = node;

  hash->count++;

  pthread_mutex_unlock( &hash->mutex[ hash_value ] );

  return 1;
}


void *
delete_hash( struct hash *hash, void *key ) {
  assert( hash != NULL );
  assert( key != NULL );

  int hash_value = -1;
  if ( ( hash_value = calculate_hash( key, hash->keylen ) ) < 0 ) {
    return NULL;
  }

  struct hashnode *prev = &hash->table[ hash_value ];

  pthread_mutex_lock( &hash->mutex[ hash_value ] );

  struct hashnode *ptr = NULL;
  for ( ptr = prev->next; ptr != NULL; ptr = ptr->next ) {
    if ( compare_key( key, ptr->key, hash->keylen ) == 0 ) {
      break;
    }
    prev = ptr;
  }

  if ( ptr == NULL ) {
    pthread_mutex_unlock( &hash->mutex[ hash_value ] );
    return NULL;
  }

  prev->next = ptr->next;
  void *data = ptr->data;
  free( ptr->key );
  free( ptr );

  hash->count--;

  pthread_mutex_unlock( &hash->mutex[ hash_value ] );

  return data;
}


void *
search_hash( struct hash *hash, void *key ) {
  assert( hash != NULL );
  assert( key != NULL );

  int hash_value;
  if ( ( hash_value = calculate_hash( key, hash->keylen ) ) < 0 ) {
    return NULL;
  }

  struct hashnode *ptr = &hash->table[ hash_value ];

  pthread_mutex_lock( &hash->mutex[ hash_value ] );

  for ( ptr = ptr->next; ptr != NULL; ptr = ptr->next ) {
    if ( compare_key( key, ptr->key, hash->keylen ) == 0 ) {
      pthread_mutex_unlock( &hash->mutex[ hash_value ] );
      return ptr->data;
    }
  }

  pthread_mutex_unlock( &hash->mutex[ hash_value ] );

  return NULL;
}


void
destroy_hash( struct hash *hash ) {
  assert( hash != NULL );

  for ( int n = 0; n < HASH_TABLE_SIZE; n++ ) {
    struct hashnode *ptr = &hash->table[ n ];
    pthread_mutex_lock( &hash->mutex[ n ] );
    if ( ptr->next != NULL ) {
      for ( struct hashnode *pptr = ptr->next; pptr != NULL; ) {
        struct hashnode *prev = pptr;
        pptr = pptr->next;
        free( prev->key );
        free( prev->data );
        free( prev );
      }
    }
    pthread_mutex_unlock( &hash->mutex[ n ] );
  }
}


void **
create_list_from_hash( struct hash *hash, int *num ) {
  assert( hash != NULL );
  assert( num != NULL );

  void **datalist = ( void ** ) malloc( sizeof( void * ) * ( size_t ) hash->count );
  memset( datalist, 0, sizeof( void * ) * ( size_t ) hash->count );

  for ( int c = 0, n = 0; n < HASH_TABLE_SIZE; n++ ) {
    struct hashnode *ptr = &hash->table[ n ];
    pthread_mutex_lock( &hash->mutex[ n ] );
    if ( ptr->next != NULL ) {
      for ( struct hashnode *pptr = ptr->next; pptr != NULL; pptr = pptr->next ) {
        // datalist[ c ] = ( void * ) malloc( sizeof( void * ) );
        datalist[ c++ ] = pptr->data;
      }
    }
    pthread_mutex_unlock( &hash->mutex[ n ] );
  }

  *num = hash->count;

  return datalist;
}



/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
