/*
 * Author: Yasunobu Chiba
 *
 * Copyright (C) 2008-2013 NEC Corporation
 * NEC Confidential
 */


#include <assert.h>
#include <stdlib.h>
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
  while ( queue->head != queue->divider ) {
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
  __sync_add_and_fetch( &queue->length, 1 ); // this must be an atomic operation for thread safety

  collect_garbage( queue );

  return true;
}


void *
peek( queue *queue ) {
  assert( queue != NULL );

  if ( queue->divider != queue->tail ) {
    return queue->divider->next->data;
  }

  return NULL;
}


void *
dequeue( queue *queue ) {
  assert( queue != NULL );

  if ( queue->divider != queue->tail ) {
    queue_element *next = queue->divider->next;
    void *data = next->data;
    next->data = NULL; // data must be freed by caller
    queue->divider = next;
    __sync_sub_and_fetch( &queue->length, 1 ); // this must be an atomic operation for thread safety

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
