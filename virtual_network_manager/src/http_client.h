/*
 * HTTP client functions to interact with REST-based interfaces
 *
 * Author: Yasunobu Chiba
 *
 * Copyright (C) 2012 NEC Corporation
 * NEC Confidential
 */


#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H


#include "trema.h"


enum {
  HTTP_METHOD_GET,
  HTTP_METHOD_POST,
  HTTP_METHOD_PUT,
  HTTP_METHOD_DELETE,
  HTTP_METHOD_MAX,
};

enum {
  HTTP_TRANSACTION_SUCCEEDED = 0,
  HTTP_TRANSACTION_FAILED = -1,
};


typedef struct {
  char content_type[ 128 ];
  buffer *body;
} http_content;

typedef void ( *request_completed_handler )(
  int status,
  int code,
  const http_content *content,
  void *user_data
);


bool do_http_request( uint8_t method, const char *uri, const http_content *content,
                      request_completed_handler completed_callback, void *completed_user_data );
bool init_http_client();
bool finalize_http_client();


#endif // HTTP_CLIENT_H


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
