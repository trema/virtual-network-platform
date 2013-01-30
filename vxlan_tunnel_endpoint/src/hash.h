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
