/*
 * Copyright (C) 2008-2013 NEC Corporation
 * NEC Confidential
 */


#ifndef LINKED_LIST_H
#define LINKED_LIST_H


#include <pthread.h>
#include <stdbool.h>


typedef struct list_element {
  struct list_element *next;
  void *data;
} list_element;


typedef struct {
  list_element *head;
  pthread_mutex_t mutex;
} list;


list *create_list();
bool insert_in_front( list *l, void *data );
bool append_to_tail( list *l, void *data );
bool delete_element( list *l, const void *data );
bool delete_list( list *l );
bool delete_list_totally( list *l );


#endif // LINKED_LIST_H


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
