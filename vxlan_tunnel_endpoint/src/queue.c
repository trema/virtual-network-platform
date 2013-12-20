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


#include <assert.h>
#include <stdlib.h>
#include "memory_barrier.h"
#include "queue.h"


queue *
create_queue( void ) {
  queue *new_queue = malloc( sizeof( queue ) );
  new_queue->head = malloc( sizeof( queue_element ) );
  new_queue->head->data = NULL;
  new_queue->head->next = NULL;
  new_queue->divider = new_queue->tail = new_queue->head;
  new_queue->length = 0;

  return new_queue;
}


bool
delete_queue( queue *queue ) {
  assert( queue != NULL );

  while( queue->head != NULL ) {
    queue_element *e = queue->head;
    if ( queue->head->data != NULL ) {
      free( queue->head->data );
      queue->head->data = NULL;
    }
    queue->head = queue->head->next;
    free( e );
  }
  free( queue );

  return true;
}


static void
collect_garbage( queue *queue ) {
  while ( 1 ) {
    volatile queue_element *divider = queue->divider;
    read_memory_barrier();
    if ( queue->head == divider ) {
      break;
    }
    queue_element *e = queue->head;
    queue->head = queue->head->next;
    free( e );
  }
}


bool
enqueue( queue *queue, void *data ) {
  assert( queue != NULL );
  assert( data != NULL );

  queue_element *new_tail = malloc( sizeof( queue_element ) );
  new_tail->data = data;
  new_tail->next = NULL;

  queue->tail->next = new_tail;
  queue->tail = new_tail;
  write_memory_barrier();
  __sync_add_and_fetch( &queue->length, 1 );

  collect_garbage( queue );

  return true;
}


void *
peek( queue *queue ) {
  assert( queue != NULL );

  volatile queue_element *divider = queue->divider;
  volatile queue_element *tail = queue->tail;
  read_memory_barrier();

  if ( divider != tail ) {
    return divider->next->data;
  }

  return NULL;
}


void *
dequeue( queue *queue ) {
  assert( queue != NULL );

  volatile queue_element *tail = queue->tail;
  read_memory_barrier();

  if ( queue->divider != tail ) {
    queue_element *next = queue->divider->next;
    void *data = next->data;
    next->data = NULL; // data must be freed by caller
    queue->divider = next;
    write_memory_barrier();
    __sync_sub_and_fetch( &queue->length, 1 );

    return data;
  }

  return NULL;
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
