/*
 * Author: Yasunobu Chiba
 *
 * Copyright (C) 2012 NEC Corporation
 * NEC Confidential
 */


#include "utilities.h"
#ifdef UNIT_TEST
#include "utilities_unittest.h"
#endif


void
free_list( list_element *head ) {
  debug( "Freeing a linked list ( %p ).", head );

  for ( list_element *e = head; e != NULL; e = e->next ) {
    if ( e->data != NULL ) {
      xfree( e->data );
    }
  }

  if ( head != NULL ) {
    delete_list( head );
  }

  debug( "A linked list is freed ( %p ).", head );
}
