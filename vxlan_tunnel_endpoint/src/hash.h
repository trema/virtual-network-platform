/*
 * Hash table implementation.
 *
 * Copryright (C) 2012 upa@haeena.net
 * Copryright (C) 2012-2013 NEC Corporation
 */


#ifndef HASH_H
#define HASH_H


#include <pthread.h>
#include <stdint.h>
#include <sys/types.h>


#define HASH_TABLE_SIZE 256


struct hashnode {
  uint8_t *key;
  void *data;
  struct hashnode *next;
};


struct hash {
  struct hashnode table[ HASH_TABLE_SIZE ];
  pthread_mutex_t mutex[ HASH_TABLE_SIZE ];
  int keylen;
  int count;
};


void init_hash( struct hash *hash, int keylen );
int insert_hash( struct hash *hash, void *data, void *key );
void *delete_hash( struct hash *hash, void *key );
void *search_hash( struct hash *hash, void *key );
void destroy_hash( struct hash *hash );
void **create_list_from_hash( struct hash *hash, int *num );


#endif // HASH_H


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
