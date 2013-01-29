/*
 * Lock-free queue implementation.
 *
 * Author: Yasunobu Chiba
 *
 * Copyright (C) 2008-2013 NEC Corporation
 * NEC Confidential
 */


#ifndef QUEUE_H
#define QUEUE_H


#include <stdbool.h>


typedef struct queue_element {
  void *data;
  struct queue_element *next;
} queue_element;

typedef struct queue {
  queue_element *head;
  queue_element *divider;
  queue_element *tail;
  int length;
} queue;


queue *create_queue( void );
bool delete_queue( queue *queue );
bool enqueue( queue *queue, void *data );
void *peek( queue *queue );
void *dequeue( queue *queue );


#endif // QUEUE_H


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
