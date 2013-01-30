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


#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "linked_list.h"


list *
create_list() {
  list *new_list = malloc( sizeof( list ) );
  assert( new_list != NULL );

  memset( new_list, 0, sizeof( list ) );
  new_list->head = NULL;

  pthread_mutexattr_t attr;
  pthread_mutexattr_init( &attr );
  pthread_mutexattr_settype( &attr, PTHREAD_MUTEX_RECURSIVE_NP );
  pthread_mutex_init( &new_list->mutex, &attr );

  return new_list;
}


bool
insert_in_front( list *l, void *data ) {
  assert( l != NULL );
  assert( data != NULL );

  pthread_mutex_lock( &l->mutex );

  list_element *old_head = l->head;
  list_element *new_head = malloc( sizeof( list_element ) );
  assert( new_head != NULL );

  new_head->data = data;
  new_head->next = old_head;
  l->head = new_head;

  pthread_mutex_unlock( &l->mutex );

  return true;
}


bool
append_to_tail( list *l, void *data ) {
  assert( l != NULL );
  assert( data != NULL );

  pthread_mutex_lock( &l->mutex );

  list_element *new_tail = malloc( sizeof( list_element ) );
  new_tail->data = data;
  new_tail->next = NULL;

  if ( l->head == NULL ) {
    l->head = new_tail;
    pthread_mutex_unlock( &l->mutex );
    return true;
  }

  list_element *e = NULL;
  for ( e = l->head; e->next != NULL; e = e->next );
  e->next = new_tail;

  pthread_mutex_unlock( &l->mutex );

  return true;
}


bool
delete_element( list *l, const void *data ) {
  assert( l != NULL );
  assert( data != NULL );

  pthread_mutex_lock( &l->mutex );

  if ( l->head != NULL && l->head->data == data ) {
    list_element *delete_me = l->head;
    l->head = l->head->next;
    free( delete_me );
    pthread_mutex_unlock( &l->mutex );

    return true;
  }

  for ( list_element *e = l->head; e->next != NULL; e = e->next ) {
    if ( e->next->data == data ) {
      list_element *delete_me = e->next;
      e->next = e->next->next;
      free( delete_me );
      pthread_mutex_unlock( &l->mutex );

      return true;
    }
  }

  pthread_mutex_unlock( &l->mutex );

  return false;
}


bool
delete_list( list *l ) {
  assert( l != NULL );

  pthread_mutex_lock( &l->mutex );

  for ( list_element *e = l->head; e != NULL; ) {
    list_element *delete_me = e;
    e = e->next;
    free( delete_me );
  }

  pthread_mutex_unlock( &l->mutex );
  pthread_mutex_destroy( &l->mutex );

  free( l );

  return true;
}


bool
delete_list_totally( list *l ) {
  if ( l == NULL ) {
    return true;
  }

  pthread_mutex_lock( &l->mutex );

  for ( list_element *e = l->head; e != NULL; e = e->next ) {
    if ( e->data != NULL ) {
      free( e->data );
      e->data = NULL;
    }
  }

  pthread_mutex_unlock( &l->mutex );

  return delete_list( l );
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
