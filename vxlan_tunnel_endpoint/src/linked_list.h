/*
 * Copyright (C) 2008-2013 NEC Corporation
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
