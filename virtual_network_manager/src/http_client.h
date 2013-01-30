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
