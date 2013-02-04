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
#include <curl/curl.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/eventfd.h>
#include <sys/select.h>
#include <unistd.h>
#include "http_client.h"
#include "trema.h"
#include "queue.h"
#ifdef UNIT_TEST
#include "http_client_unittest.h"
#endif


#define USER_AGENT "Bisco/0.0.3"


typedef struct {
  uint8_t method;
  char uri[ 1024 ];
  http_content content;
} http_request;

typedef struct {
  int status;
  int code;
  http_content content;
} http_response;

typedef struct {
  CURL *handle;
  struct curl_slist *slist;
  http_request request;
  http_response response;
  request_completed_handler callback;
  void *user_data;
} http_transaction;


static pthread_t *http_client_thread = NULL;
static int http_client_efd = -1;
static int main_efd = -1;
static queue *main_to_http_client_queue = NULL;
static queue *http_client_to_main_queue = NULL;
static hash_table *transactions = NULL;
static CURLM *curl_handle = NULL;
static int curl_running = 0;
static uint64_t http_client_notify_count = 0;
static uint64_t main_notify_count = 0;


static bool
compare_http_transaction( const void *x, const void *y ) {
  const http_transaction *transaction_x = x;
  const http_transaction *transaction_y = y;

  return ( transaction_x->handle == transaction_y->handle ) ? true : false;
}


static unsigned int
hash_http_transaction( const void *key ) {
  return ( unsigned int ) ( uintptr_t ) key;
}


static void
notify_main_actually() {
  debug( "Notifying main thread from HTTP client ( main_notify_count = %" PRIu64 " ).",
         main_notify_count );

  assert( main_efd >= 0 );

  if ( main_notify_count == 0 ) {
    error( "Notify count must not be zero." );
    return;
  }

  ssize_t ret = write( main_efd, &main_notify_count, sizeof( uint64_t ) );
  if ( ret < 0 ) {
    if ( errno == EAGAIN || errno == EINTR ) {
      return;
    }
    char buf[ 256 ];
    memset( buf, '\0', sizeof( buf ) );
    char *error_string = strerror_r( errno, buf, sizeof( buf ) - 1 );
    error( "Failed to notify an event to main thread ( main_notify_count = %" PRIu64 ", main_efd = %d, ret = %zd, errno = %s [%d] ).",
           main_notify_count, main_efd, ret, error_string, errno );
    return;
  }
  else if ( ret != sizeof( uint64_t ) ) {
    error( "Failed to notify an event to main thread ( main_notify_count = %" PRIu64 ", main_efd = %d, ret = %zd ).",
           main_notify_count, main_efd, ret );
    return;
  }

  main_notify_count = 0;

  debug( "Notification completed." );
}


static bool
notify_main() {
  debug( "Incrementing main notify count ( %" PRIu64 " ).", main_notify_count );

  main_notify_count++;

  return true;
}


static void
notify_http_client_actually( int fd, void *user_data ) {
  uint64_t *count = user_data;

  debug( "Notifying HTTP client from main thread ( fd = %d, count = %" PRIu64 " ).", fd, count != NULL ? *count : 0 );

  assert( fd >= 0 );
  assert( user_data != NULL );

  ssize_t ret = write( fd, count, sizeof( uint64_t ) );
  if ( ret < 0 ) {
    if ( errno == EAGAIN || errno == EINTR ) {
      return;
    }
    char buf[ 256 ];
    memset( buf, '\0', sizeof( buf ) );
    char *error_string = strerror_r( errno, buf, sizeof( buf ) - 1 );
    error( "Failed to notify an event to HTTP client ( count = %" PRIu64 ", fd = %d, ret = %zd, errno = %s [%d] ).",
           *count, fd, ret, error_string, errno );
    return;
  }
  else if ( ret != sizeof( uint64_t ) ) {
    error( "Failed to notify an event to HTTP client ( count = %" PRIu64 ", fd = %d, ret = %zd ).", *count, fd, ret );
  }

  set_writable( fd, false );
  *count = 0;

  debug( "Notification completed." );
}


static bool
notify_http_client() {
  debug( "Incrementing HTTP client notify count ( %" PRIu64 " ).", http_client_notify_count );

  assert( http_client_efd >= 0 );

  http_client_notify_count++;
  set_writable( http_client_efd, true );

  return true;
}


static int
create_event_fd() {
  debug( "Creating an event fd." );

  int efd = eventfd( 0, EFD_NONBLOCK );
  if ( efd < 0 ) {
    char buf[ 256 ];
    memset( buf, '\0', sizeof( buf ) );
    char *error_string = strerror_r( errno, buf, sizeof( buf ) - 1 );
    error( "Failed to create an event fd ( errno = %s [%d] ).", error_string, errno );
    return -1;
  }

  debug( "An event fd is created ( fd = %d ).", efd );

  return efd;
}


static http_transaction *
alloc_http_transaction() {
  debug( "Allocating a HTTP transaction record." );

  http_transaction *transaction = xmalloc( sizeof( http_transaction ) );
  memset( transaction, 0, sizeof( http_transaction ) );

  debug( "A HTTP transaction is allocated ( %p ).", transaction );

  return transaction;
}


static void
free_http_transaction( http_transaction *transaction ) {
  debug( "Freeing a HTTP transaction record ( %p ).", transaction );

  assert( transaction != NULL );

  if ( transaction->handle != NULL ) {
    debug( "Cleaning up curl easy handle ( %p ).", transaction->handle );
    curl_easy_cleanup( transaction->handle );
  }
  if ( transaction->slist != NULL ) {
    debug( "Cleaning up curl slist ( %p ).", transaction->slist );
    curl_slist_free_all( transaction->slist );
  }
  if ( transaction->request.content.body != NULL ) {
    debug( "Cleaning up request content body ( %p ).", transaction->request.content.body );
    free_buffer( transaction->request.content.body );
  }
  if ( transaction->response.content.body != NULL ) {
    debug( "Cleaning up response content body ( %p ).", transaction->response.content.body );
    free_buffer( transaction->response.content.body );
  }

  xfree( transaction );

  debug( "A HTTP transaction record is freed ( %p ).", transaction );
}


static bool
add_http_transaction( http_transaction *transaction, CURL *handle ) {
  debug( "Adding a HTTP transaction to database ( transaction = %p, handle = %p ).",
         transaction, handle );

  assert( transaction != NULL );
  assert( handle != NULL );

  transaction->handle = handle;

  assert( curl_handle != NULL );
  CURLMcode ret = curl_multi_add_handle( curl_handle, transaction->handle );
  if ( ret != CURLM_OK && ret != CURLM_CALL_MULTI_PERFORM ) {
    error( "Failed to add an easy handle to a multi handle ( curl_handle = %p, handle = %p, error = %s ).",
           curl_handle, transaction->handle, curl_multi_strerror( ret ) );
    return false;
  }
  ret = curl_multi_perform( curl_handle, &curl_running );
  if ( ret != CURLM_OK && ret != CURLM_CALL_MULTI_PERFORM ) {
    error( "Failed to run a multi handle ( curl_handle = %p, error = %s ).", curl_handle, curl_multi_strerror( ret ) );
    ret = curl_multi_remove_handle( curl_handle, transaction->handle );
    if ( ret != CURLM_OK  && ret != CURLM_CALL_MULTI_PERFORM ) {
      error( "Failed to remove an easy handle from a multi handle ( curl_handle = %p, handle = %p, error = %s ).",
             curl_handle, transaction->handle, curl_multi_strerror( ret ) );             
    }
    return false;
  }
  assert( transactions != NULL );

  void *duplicated = insert_hash_entry( transactions, transaction->handle, transaction );
  if ( duplicated != NULL ) {
    // FIXME: handle duplication properly
    warn( "Duplicated HTTP transaction found ( %p ).", duplicated );
    free_http_transaction( duplicated );
  }

  debug( "A HTTP transaction is added into database ( transaction = %p, handle = %p ).", transaction, handle );

  return true;
}


static bool
delete_http_transaction( http_transaction *transaction ) {
  debug( "Deleting a HTTP transaction from database ( transaction = %p, handle = %p ).",
         transaction, transaction->handle );

  assert( transaction != NULL );
  assert( transaction->handle != NULL );

  assert( transactions != NULL );
  void *deleted = delete_hash_entry( transactions, transaction->handle );
  if ( deleted == NULL ) {
    error( "Failed to delete a HTTP transaction ( transaction = %p, handle = %p ).",
           transaction, transaction->handle );
    return false;
  }
  assert( deleted == transaction );

  assert( curl_handle != NULL );
  CURLMcode ret = curl_multi_remove_handle( curl_handle, transaction->handle );
  if ( ret != CURLM_OK  && ret != CURLM_CALL_MULTI_PERFORM ) {
    error( "Failed to remove an easy handle from a multi handle ( curl_handle = %p, handle = %p, error = %s ).",
           curl_handle, transaction->handle, curl_multi_strerror( ret ) );
  }
  if ( transaction->slist != NULL ) {
    curl_slist_free_all( transaction->slist );
    transaction->slist = NULL;
  }
  curl_easy_cleanup( transaction->handle );
  transaction->handle = NULL;

  debug( "A HTTP transaction is deleted from database ( transaction = %p, handle = %p ).", transaction, transaction->handle );

  return true;
}


static http_transaction *
lookup_http_transaction( CURL *easy_handle ) {
  debug( "Looking up a HTTP transaction ( easy_handle = %p, transactions = %p ).", easy_handle, transactions );

  assert( easy_handle != NULL );
  assert( transactions != NULL );

  http_transaction *transaction = lookup_hash_entry( transactions, easy_handle );
  if ( transaction == NULL ) {
    debug( "Failed to lookup a transaction ( transactions = %p, easy_handle = %p ).", transactions, easy_handle );
    return NULL;
  }

  debug( "Transaction found ( transaction = %p, transactions = %p, easy_handle = %p ).", transaction, transactions, easy_handle );

  return transaction;
}


static void
create_http_transaction_db() {
  debug( "Creating HTTP transaction database." );

  assert( transactions == NULL );

  transactions = create_hash( compare_http_transaction, hash_http_transaction );
  assert( transactions != NULL );

  debug( "HTTP transaction database is created ( %p ).", transactions );
}


static void
delete_http_transaction_db() {
  debug( "Deleting HTTP transaction database ( %p ).", transactions );

  assert( transactions != NULL );

  hash_entry *e = NULL;
  hash_iterator iter;
  init_hash_iterator( transactions, &iter );
  while ( ( e = iterate_hash_next( &iter ) ) != NULL ) {
    if ( e->value != NULL ) {
      free_http_transaction( e->value );
      e->value = NULL;
    }
  }

  delete_hash( transactions );

  debug( "HTTP transaction database is deleted ( %p ).", transactions );

  transactions = NULL;
}


static size_t
write_to_buffer( void *contents, size_t size, size_t nmemb, void *user_data ) {
  debug( "Writing contents ( contents = %p, size = %zu, nmemb = %zu, user_data = %p ).",
         contents, size, nmemb, user_data );

  assert( user_data != NULL );

  size_t real_size = size * nmemb;
  buffer *buf = user_data;
  void *p = append_back_buffer( buf, real_size );
  memcpy( p, contents, real_size );

  debug( "%zu bytes data is written into buffer ( buf = %p, length = %zu ).", real_size, buf, buf->length );

  return real_size;
}


static size_t
read_from_buffer( void *ptr, size_t size, size_t nmemb, void *user_data ) {
  debug( "Reading contents ( ptr = %p, size = %zu, nmemb = %zu, user_data = %p ).",
        ptr, size, nmemb, user_data );

  assert( user_data != NULL );

  buffer *buf = user_data;
  if ( buf == NULL || ( buf != NULL && buf->length == 0 ) ) {
    debug( "buffer is empty ( buf = %p, length = %zu ).", buf, buf != NULL ? buf->length : 0 );
    return 0;
  }

  size_t real_size = size * nmemb;
  if ( real_size == 0 ) {
    debug( "Nothing to read." );
    return 0;
  }

  if ( buf->length < real_size ) {
    real_size = buf->length;
  }
  memcpy( ptr, buf->data, real_size );
  remove_front_buffer( buf, real_size );

  debug( "%zu bytes data is read from buffer ( buf = %p, length = %zu ).", real_size, buf, buf->length );

  return real_size;
}


static bool
set_contents( CURL *handle, http_transaction *transaction ) {
  debug( "Setting contents to an easy handle ( handle = %p, transaction = %p ).", handle, transaction );

  assert( handle != NULL );
  assert( transaction != NULL );

  http_request *request = &transaction->request;

  if ( request->content.body == NULL || ( request->content.body != NULL && request->content.body->length == 0 ) ) {
    return true;
  }

  if ( strlen( request->content.content_type ) > 0 ) {
    char content_type[ 128 ];
    memset( content_type, '\0', sizeof( content_type ) );
    snprintf( content_type, sizeof( content_type ), "Content-Type: %s", request->content.content_type );
    transaction->slist = curl_slist_append( transaction->slist, content_type );
  }
  else {
    transaction->slist = curl_slist_append( transaction->slist, "Content-Type: text/plain" );
  }
  CURLcode ret = curl_easy_setopt( handle, CURLOPT_READFUNCTION, read_from_buffer );
  if ( ret != CURLE_OK ) {
    error( "Failed to set a read function ( handle = %p, error = %s ).", handle, curl_easy_strerror( ret ) );
    return false;
  }
  ret = curl_easy_setopt( handle, CURLOPT_READDATA, ( void * ) request->content.body );
  if ( ret != CURLE_OK ) {
    error( "Failed to set a read data ( handle = %p, error = %s ).", handle, curl_easy_strerror( ret ) );
    return false;
  }

  return true;
}


static void
handle_http_transaction_from_main( http_transaction *transaction ) {
  assert( transaction != NULL );

  http_request *request = &transaction->request;
  uint32_t content_length = 0;
  if ( request->content.body != NULL && request->content.body->length > 0 ) {
    content_length = ( uint32_t ) --request->content.body->length; // We only accept null-terminated string.
  }

  debug( "Handling a transaction from main thread ( method = %#x, uri = %s, content_length = %u, content_type = %s, content_body = %p ).",
         request->method, request->uri, content_length, request->content.content_type, request->content.body );

  transaction->slist = NULL;
  CURL *handle = curl_easy_init();
  CURLcode retval = CURLE_OK;
  switch ( request->method ) {
    case HTTP_METHOD_GET:
    {
      retval = curl_easy_setopt( handle, CURLOPT_HTTPGET, 1 );
      if ( retval != CURLE_OK ) {
        error( "Failed to set HTTP method to GET ( handle = %p, error = %s ).", handle, curl_easy_strerror( retval ) );
        goto error;
      }
    }
    break;

    case HTTP_METHOD_PUT:
    {
      retval = curl_easy_setopt( handle, CURLOPT_UPLOAD, 1 );
      if ( retval != CURLE_OK ) {
        error( "Failed to set HTTP method to PUT ( handle = %p, error = %s ).", handle, curl_easy_strerror( retval ) );
        goto error;
      }
      retval = curl_easy_setopt( handle, CURLOPT_INFILESIZE, content_length );
      if ( retval != CURLE_OK ) {
        error( "Failed to set content length value ( content_length = %u, handle = %p, error = %s ).",
               content_length, handle, curl_easy_strerror( retval ) );
        goto error;
      }
      transaction->slist = curl_slist_append( transaction->slist, "Expect:" );
    }
    break;

    case HTTP_METHOD_POST:
    {
      retval = curl_easy_setopt( handle, CURLOPT_POST, 1 );
      if ( retval != CURLE_OK ) {
        error( "Failed to set HTTP method to POST ( handle = %p, error = %s ).", handle, curl_easy_strerror( retval ) );
        goto error;
      }
      retval = curl_easy_setopt( handle, CURLOPT_POSTFIELDSIZE, content_length );
      if ( retval != CURLE_OK ) {
        error( "Failed to set content length value ( content_length = %u, handle = %p, error = %s ).",
               content_length, handle, curl_easy_strerror( retval ) );
        goto error;
      }
    }
    break;

    case HTTP_METHOD_DELETE:
    {
      retval = curl_easy_setopt( handle, CURLOPT_CUSTOMREQUEST, "DELETE" );
      if ( retval != CURLE_OK ) {
        error( "Failed to set HTTP method to DELETE ( handle = %p, error = %s ).", handle, curl_easy_strerror( retval ) );
        goto error;
      }
    }
    break;

    default:
    {
      error( "Undefined HTTP request method ( %u ).", request->method );
      assert( 0 );
    }
    break;
  }

  bool ret = set_contents( handle, transaction );
  if ( !ret ) {
    goto error;
  }

  retval = curl_easy_setopt( handle, CURLOPT_USERAGENT, USER_AGENT );
  if ( retval != CURLE_OK ) {
    error( "Failed to set User-Agent header field value ( handle = %p, transaction = %p, error = %s ).",
           handle, transaction, curl_easy_strerror( retval ) );
    goto error;
  }
  retval = curl_easy_setopt( handle, CURLOPT_URL, request->uri );
  if ( retval != CURLE_OK ) {
    error( "Failed to set URL value ( uri = %s, handle = %p, transaction = %p, error = %s ).",
           request->uri, handle, transaction, curl_easy_strerror( retval ) );
    goto error;
  }
  retval = curl_easy_setopt( handle, CURLOPT_NOPROGRESS, 1 );
  if ( retval != CURLE_OK ) {
    error( "Failed to enable NOPROGRESS option ( handle = %p, transaction = %p, error = %s ).",
           handle, transaction, curl_easy_strerror( retval ) );
    goto error;
  }
  retval = curl_easy_setopt( handle, CURLOPT_WRITEFUNCTION, write_to_buffer );
  if ( retval != CURLE_OK ) {
    error( "Failed to set a write function ( handle = %p, transaction = %p, error = %s ).",
           handle, transaction, curl_easy_strerror( retval ) );
    goto error;
  }
  http_response *response = &transaction->response;
  response->content.body = alloc_buffer_with_length( 1024 );
  retval = curl_easy_setopt( handle, CURLOPT_WRITEDATA, ( void * ) response->content.body );
  if ( retval != CURLE_OK ) {
    error( "Failed to set a write data ( handle = %p, transaction = %p, error = %s ).",
           handle, transaction, curl_easy_strerror( retval ) );
    goto error;
  }
  if ( transaction->slist != NULL ) {
    retval = curl_easy_setopt( handle, CURLOPT_HTTPHEADER, transaction->slist );
    if ( retval != CURLE_OK ) {
      error( "Failed to set custom header field values ( handle = %p, transaction = %p, slist = %p, error = %s ).",
             handle, transaction, transaction->slist, curl_easy_strerror( retval ) );
      goto error;
    }
  }

  add_http_transaction( transaction, handle );

  return;

error:
  if ( handle != NULL ) {
    curl_easy_cleanup( handle );
  }
  if ( transaction->slist != NULL ) {
    curl_slist_free_all( transaction->slist );
    transaction->slist = NULL;
  }
  response = &transaction->response;
  response->status = HTTP_TRANSACTION_FAILED;
  response->code = 0;
  enqueue( http_client_to_main_queue, transaction );
  notify_main();
}


static void
retrieve_http_transactions_from_main() {
  debug( "Retrieving HTTP transactions from main thread." );

  assert( http_client_efd >= 0 );

  uint64_t count = 0;
  ssize_t ret = read( http_client_efd, &count, sizeof( uint64_t ) );
  if ( ret < 0 ) {
    if ( errno == EAGAIN || errno == EINTR ) {
      return;
    }
    char buf[ 256 ];
    memset( buf, '\0', sizeof( buf ) );
    char *error_string = strerror_r( errno, buf, sizeof( buf ) - 1 );
    error( "Failed to read an event from HTTP client thread ( http_client_efd = %d, ret = %zd, errno = %s [%d] ).",
           http_client_efd, ret, error_string, errno );
  }
  else if ( ret != sizeof( uint64_t ) ) {
    error( "Failed to read an event from HTTP client thread ( http_client_efd = %d, ret = %zd ).",
           http_client_efd, ret );
    return;
  }

  debug( "%" PRIu64 " transactions need to be retrieved from main thread.", count );

  for ( uint64_t i = 0; i < count; i++ ) {
    http_transaction *transaction = dequeue( main_to_http_client_queue );
    if ( transaction == NULL ) {
      break;
    }
    handle_http_transaction_from_main( transaction );
  }
}


static void
retrieve_http_transactions_from_http_client( int fd, void *user_data ) {
  debug( "Retrieving HTTP transactions from HTTP client ( fd = %d, user_data = %p ).", fd, user_data );

  assert( fd >= 0 );

  uint64_t count = 0;
  ssize_t ret = read( fd, &count, sizeof( uint64_t ) );
  if ( ret < 0 ) {
    if ( errno == EAGAIN || errno == EINTR ) {
      return;
    }
    char buf[ 256 ];
    memset( buf, '\0', sizeof( buf ) );
    char *error_string = strerror_r( errno, buf, sizeof( buf ) - 1 );
    error( "Failed to read an event from HTTP client thread ( fd = %d, ret = %zd, errno = %s [%d] ).",
           fd, ret, error_string, errno );
  }
  else if ( ret != sizeof( uint64_t ) ) {
    error( "Failed to read an event from HTTP client thread ( fd = %d, ret = %zd ).", fd, ret );
    return;
  }

  debug( "%" PRIu64 " transactions need to be retrieved from HTTP client.", count );

  for ( uint64_t i = 0; i < count; i++ ) {
    http_transaction *transaction = dequeue( http_client_to_main_queue );
    if ( transaction == NULL ) {
      break;
    }

    http_request *request = &transaction->request;
    assert( request != NULL );
    http_response *response = &transaction->response;
    assert( response != NULL );

    debug( "Handling a transaction from HTTP client ( "
           "request = [method = %#x, uri = %s, content_length = %zu, content_type = %s, content_body = %p], "
           "response = [status = %d, code = %d, content_length = %zu, content_type = %s, content_body = %p], "
           "handle = %p, slist = %p, callback = %p, user_data = %p ).",
           request->method, request->uri, request->content.body != NULL ? request->content.body->length : 0,
           request->content.content_type, request->content.body,
           response->status, response->code, response->content.body != NULL ? response->content.body->length : 0,
           response->content.content_type, response->content.body,
           transaction->handle, transaction->slist, transaction->callback, transaction->user_data );

    if ( transaction->callback != NULL ) {
      http_content *content = NULL;
      if ( response->content.body != NULL && response->content.body->length > 0 ) {
        content = &response->content;
      }
      transaction->callback( response->status, response->code, content, transaction->user_data );
    }
    free_http_transaction( transaction );
  }
}


static bool
send_http_transaction_to_main( http_transaction *transaction, CURLMsg *msg ) {
  debug( "Sending a HTTP transaction to main thread ( transaction %p, msg = %p ).", transaction, msg );

  assert( transaction != NULL );
  assert( msg != NULL );

  bool ret = true;
  assert( transaction->handle != NULL );
  CURL *easy_handle = transaction->handle;
  http_response *response = &transaction->response;
  if ( msg->data.result == CURLE_OK ) {
    // Set return status value and HTTP status code
    uint32_t code = 0;
    CURLcode status = curl_easy_getinfo( easy_handle, CURLINFO_RESPONSE_CODE, &code );
    if ( status == CURLE_OK ) {
      debug( "Status code = %u.", code );
      response->code = ( int ) code;
    }
    else {
      error( "Failed to get response code ( easy_handle = %p ).", easy_handle );
      response->code = 0;
      ret &= false;
    }
    if ( code >= 200 && code < 300 ) {
      response->status = HTTP_TRANSACTION_SUCCEEDED;
    }
    else { 
      response->status = HTTP_TRANSACTION_FAILED;
    }

    // Set Content-Type value
    memset( response->content.content_type, '\0', sizeof( response->content.content_type ) );
    char *content_type = NULL;
    status = curl_easy_getinfo( easy_handle, CURLINFO_CONTENT_TYPE, &content_type );
    if ( status == CURLE_OK ) {
      if ( content_type != NULL ) {
        strncpy( response->content.content_type, content_type, sizeof( response->content.content_type ) - 1 );
      }
    }
    else {
      error( "Failed to get Content-Type value ( easy_handle = %p ).", easy_handle );
      ret &= false;
    }
  }
  else {
    error( "Failed to execute a HTTP transaction ( %s ).", curl_easy_strerror( msg->data.result ) );
    response->status = HTTP_TRANSACTION_FAILED;
    response->code = 0;
  }

  ret &= delete_http_transaction( transaction );
  ret &= enqueue( http_client_to_main_queue, transaction );
  ret &= notify_main();

  return ret;
}


static bool
send_http_transaction_to_http_client( http_transaction *transaction ) {
  debug( "Sending a HTTP transaction to HTTP client ( %p ).", transaction );

  assert( transaction != NULL );
  assert( main_to_http_client_queue != NULL );

  bool ret = enqueue( main_to_http_client_queue, transaction );
  ret &= notify_http_client();

  return ret;
}


static bool
init_curl() {
  debug( "Initializing curl." );

  assert( curl_handle == NULL );

  curl_handle = curl_multi_init();
  assert( curl_handle != NULL );

  debug ( "Initialization completed ( %p ).", curl_handle );

  return true;
}


static bool
finalize_curl() {
  debug( "Finalizing curl ( %p ).", curl_handle );

  assert( curl_handle != NULL );

  curl_multi_cleanup( curl_handle );

  debug( "Finalization completed( %p ).", curl_handle );

  curl_handle = NULL;

  return true;
}


static void *
http_client_main( void *args ) {
  UNUSED( args );

  debug( "Starting HTTP client thread ( tid = %#lx ).", pthread_self() );

  init_curl();

  while ( 1 ) {
    int n_msgs = 0;
    CURLMsg *msg = curl_multi_info_read( curl_handle, &n_msgs );
    while ( msg != NULL ) {
      if ( msg->msg != CURLMSG_DONE ) {
        continue;
      }
      http_transaction *transaction = lookup_http_transaction( msg->easy_handle );
      if ( transaction != NULL ) {
        send_http_transaction_to_main( transaction, msg );
      }
      else {
        warn( "Failed to find HTTP transaction record ( easy_handle = %p ).", msg->easy_handle );
      }
      msg = curl_multi_info_read( curl_handle, &n_msgs );
    }

    fd_set fd_read;
    FD_ZERO( &fd_read );
    fd_set fd_write;
    FD_ZERO( &fd_write );
    fd_set fd_excep;
    FD_ZERO( &fd_excep );

    int fd_max = -1;
    long curl_timeout = -1;
     struct timespec timeout = { 10, 0 };
    if ( curl_running ) {
      CURLMcode retval = curl_multi_timeout( curl_handle, &curl_timeout );
      if ( retval != CURLM_OK && retval != CURLM_CALL_MULTI_PERFORM ) {
        error( "Failed to get timeout value from a handle ( handle = %p, error = %s ).", curl_handle, curl_multi_strerror( retval ) );
      }
      if ( curl_timeout >= 0 ) {
        timeout.tv_sec = curl_timeout / 1000;
        if ( timeout.tv_sec > 1 ) {
          timeout.tv_sec = 1;
        }
        else {
          timeout.tv_nsec = ( curl_timeout % 1000 ) * 1000000;
        }
      }
 
      retval = curl_multi_fdset( curl_handle, &fd_read, &fd_write, &fd_excep, &fd_max );
      if ( retval != CURLM_OK && retval != CURLM_CALL_MULTI_PERFORM ) {
        error( "Failed to retrieve fd set from a handle ( handle = %p, error = %s ).", curl_handle, curl_multi_strerror( retval ) );
      }
    }

    if ( http_client_efd >= 0 ) {
#define sizeof( _x ) ( ( int ) sizeof( _x ) )
      FD_SET( http_client_efd, &fd_read );
#undef sizeof
      if ( http_client_efd > fd_max ) {
        fd_max = http_client_efd;
      }
    }
    if ( main_efd >= 0 && main_notify_count > 0 ) {
#define sizeof( _x ) ( ( int ) sizeof( _x ) )
      FD_SET( main_efd, &fd_write );
#undef sizeof
      if ( main_efd > fd_max ) {
        fd_max = main_efd;
      }
    }

    int ret = pselect( fd_max + 1, &fd_read, &fd_write, &fd_excep, &timeout, NULL );
    switch ( ret ) {
      case -1:
      {
        if ( errno == EINTR ) {
          continue;
        }
        char buf[ 256 ];
        memset( buf, '\0', sizeof( buf ) );
        char *error_string = strerror_r( errno, buf, sizeof( buf ) - 1 );
        warn( "Failed to select ( errno = %s [%d] ).", error_string, errno );
      }
      break;

      case 0:
      default:
      {
        if ( curl_running ) {
          CURLMcode retval = curl_multi_perform( curl_handle, &curl_running );
          if ( retval != CURLM_OK && retval != CURLM_CALL_MULTI_PERFORM ) {
            error( "Failed to run a multi handle ( handle = %p, error = %s ).", curl_handle, curl_multi_strerror( retval ) );
          }
        }
#define sizeof( _x ) ( ( int ) sizeof( _x ) )
        if ( http_client_efd >= 0 && FD_ISSET( http_client_efd, &fd_read ) ) {
          retrieve_http_transactions_from_main();
        }
        if ( main_efd >= 0 && FD_ISSET( main_efd, &fd_write ) ) {
          notify_main_actually();
        }
#undef sizeof
      }
      break;
    }
  }

  finalize_curl();

  debug( "Stopping HTTP client thread ( tid = %#lx ).", pthread_self() );

  return NULL;
}


void
maybe_create_http_client_thread() {
  debug( "Creating a HTTP client thread." );

  if ( http_client_thread != NULL ) {
    int ret = pthread_kill( *http_client_thread, 0 );
    if ( ret == 0 ) {
      debug( "HTTP client thread is already created and active ( %#lx ).", *http_client_thread );
      return;
    }
    warn( "A HTTP client thread has terminated unexpectedly ( %#lx ). Creating a new thread.", *http_client_thread );
    xfree( http_client_thread );
    http_client_thread = NULL;
  }

  pthread_attr_t attr;
  pthread_attr_init( &attr );
  pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_DETACHED );
  http_client_thread = xmalloc( sizeof( pthread_t ) );
  int ret = pthread_create( http_client_thread, &attr, http_client_main, NULL );
  if ( ret != 0 ) {
    critical( "Failed to create a HTTP client thread ( ret = %d ).", ret );
  }
  assert( http_client_thread != NULL );

  debug( "A HTTP client is created ( %#lx ).", *http_client_thread );
}


bool
do_http_request( uint8_t method, const char *uri, const http_content *content,
                 request_completed_handler completed_callback, void *completed_user_data ) {
  debug( "Sending a HTTP request to '%s' ( method = %u, content = %p, complated_callback = %p, completed_user_data = %p ).",
         uri, method, content, completed_callback, completed_user_data );

  assert( method < HTTP_METHOD_MAX );
  assert( uri != NULL );

  maybe_create_http_client_thread();

  http_transaction *transaction = alloc_http_transaction();

  http_request *request = &transaction->request;
  request->method = method;
  memset( request->uri, '\0', sizeof( request->uri ) );
  strncpy( request->uri, uri, sizeof( request->uri ) - 1 );
  if ( content != NULL ) {
    request->content = *content;
    if ( content->body != NULL ) {
      request->content.body = duplicate_buffer( content->body );
    }
    else {
      request->content.body = NULL;
    }
  }
  else {
    memset( request->content.content_type, '\0', sizeof( request->content.content_type ) );
    request->content.body = NULL;
  }
  transaction->callback = completed_callback;
  transaction->user_data = completed_user_data;

  return send_http_transaction_to_http_client( transaction );
}


bool
init_http_client() {
  debug( "Initializaing HTTP client." );

  assert( http_client_thread == NULL );
  assert( http_client_efd == -1 );
  assert( main_efd == -1 );
  assert( transactions == NULL );

  http_client_efd = create_event_fd();
  assert( http_client_efd >= 0 );
  http_client_notify_count = 0;
  set_fd_handler( http_client_efd, NULL, NULL, notify_http_client_actually, &http_client_notify_count );
  set_writable( http_client_efd, false );

  main_efd = create_event_fd();
  assert( main_efd >= 0 );
  set_fd_handler( main_efd, retrieve_http_transactions_from_http_client, NULL, NULL, NULL );
  set_readable( main_efd, true );

  assert( main_to_http_client_queue == NULL );
  assert( http_client_to_main_queue == NULL );

  main_to_http_client_queue = create_queue();
  assert( main_to_http_client_queue != NULL );
  http_client_to_main_queue = create_queue();
  assert( http_client_to_main_queue != NULL );

  create_http_transaction_db();

  debug( "Initialization completed." );

  return true;
}


bool
finalize_http_client() {
  debug( "Finalizaing HTTP client ( transactions = %p ).", transactions );

  assert( transactions != NULL );

  if ( http_client_thread != NULL ) {
    pthread_cancel( *http_client_thread );
    xfree( http_client_thread );
    http_client_thread = NULL;
  }

  if ( http_client_efd >= 0 ) {
    set_writable( http_client_efd, false );
    delete_fd_handler( http_client_efd );
    close( http_client_efd );
    http_client_efd = -1;
  }
  http_client_notify_count = 0;

  if ( main_efd >= 0 ) {
    set_readable( main_efd, false );
    delete_fd_handler( main_efd );
    close( main_efd );
    main_efd = -1;
  }

  delete_queue( main_to_http_client_queue );
  main_to_http_client_queue = NULL;
  delete_queue( http_client_to_main_queue );
  http_client_to_main_queue = NULL;

  delete_http_transaction_db();

  debug( "Finalization completed." );

  return true;
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
