/*
 * Author: Yasunobu Chiba
 *
 * Copyright (C) 2008-2012 NEC Corporation
 * NEC Confidential
 */


#include <assert.h>
#include "queue.h"
#ifdef UNIT_TEST
#include "queue_unittest.h"
#endif


queue *
create_queue( void ) {
  debug( "Creating a queue." );

  queue *new_queue = xmalloc( sizeof( queue ) );
  new_queue->head = xmalloc( sizeof( queue_element ) );
  new_queue->head->data = NULL;
  new_queue->head->next = NULL;
  new_queue->divider = new_queue->tail = new_queue->head;
  new_queue->length = 0;

  debug( "Queue creation completed ( %p ).", new_queue );

  return new_queue;
}


bool
delete_queue( queue *queue ) {
  debug( "Deleting a queue ( queue = %p, length = %d ).", queue, queue != NULL ? queue->length : 0 );

  assert( queue != NULL );
  assert( queue->length >= 0 );

  while( queue->head != NULL ) {
    queue_element *e = queue->head;
    if ( queue->head->data != NULL ) {
      debug( "Freeing a data from a queue ( data = %p, queue = %p ).", queue->head->data, queue );
      xfree( queue->head->data );
      queue->head->data = NULL;
    }
    queue->head = queue->head->next;
    xfree( e );
  }
  xfree( queue );

  debug( "Queue deletion completed ( %p ).", queue );

  return true;
}


static void
collect_garbage( queue *queue ) {
  debug( "Collecting garbage in a queue ( queue = %p, length = %d ).", queue, queue != NULL ? queue->length : 0 );

  assert( queue != NULL );
  assert( queue->length >= 0 );

  while ( queue->head != queue->divider ) {
    queue_element *e = queue->head;
    queue->head = queue->head->next;
    xfree( e );
  }

  debug( "Garbage collection completed ( queue = %p, length = %d ).", queue, queue->length );
}


bool
enqueue( queue *queue, void *data ) {
  debug( "Enqueueing a data to a queue ( data = %p, queue = %p, length = %d ).", data, queue, queue != NULL ? queue->length : 0 );

  assert( queue != NULL );
  assert( queue->length >= 0 );
  assert( data != NULL );

  queue_element *new_tail = xmalloc( sizeof( queue_element ) );
  new_tail->data = data;
  new_tail->next = NULL;

  queue->tail->next = new_tail;
  queue->tail = new_tail;
  __sync_add_and_fetch( &queue->length, 1 );

  collect_garbage( queue );

  debug( "Enqueue completed ( data = %p, queue = %p, length = %d ).", data, queue, queue->length );

  return true;
}


void *
dequeue( queue *queue ) {
  debug( "Dequeueing a data from a queue ( queue = %p, length = %d ).", queue, queue != NULL ? queue->length : 0 );

  assert( queue != NULL );
  assert( queue->length >= 0 );

  if ( queue->divider != queue->tail ) {
    queue_element *next = queue->divider->next;
    void *data = next->data;
    next->data = NULL; // data must be freed by caller
    queue->divider = next;
    __sync_sub_and_fetch( &queue->length, 1 );

    debug( "Dequeue completed ( queue = %p, length = %d, data = %p ).", queue, queue->length, data );

    return data;
  }

  debug( "Dequeue completed ( queue = %p, length = %d, data = %p ).", queue, queue->length, NULL );

  return NULL;
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
