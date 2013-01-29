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


#ifndef REFLECTOR_CTRL_COMMON_H
#define REFLECTOR_CTRL_COMMON_H


#include "ctrl_if.h"
#include "reflector_common.h"


#define CTRL_SERVER_SOCK_FILE "/tmp/.reflectord"

#define VNI_ANY UINT32_MAX


enum {
  ADD_TEP_REQUEST,
  ADD_TEP_REPLY,
  SET_TEP_REQUEST,
  SET_TEP_REPLY,
  DEL_TEP_REQUEST,
  DEL_TEP_REPLY,
  LIST_TEP_REQUEST,
  LIST_TEP_REPLY,
  MESSAGE_TYPE_MAX,
};

enum {
  SET_TEP_VNI = 0x0001,
  SET_TEP_IP_ADDR = 0x0002,
  SET_TEP_PORT = 0x0004,
};


typedef struct {
  command_request_header header;
  uint32_t vni;
  struct in_addr ip_addr;
  uint16_t port;
} add_tep_request;

typedef struct {
  command_request_header header;
  uint32_t vni;
  struct in_addr ip_addr;
  uint16_t set_bitmap;
  uint16_t port;
} set_tep_request;

typedef struct {
  command_request_header header;
  uint32_t vni;
  struct in_addr ip_addr;
} del_tep_request;

typedef struct {
  command_request_header header;
  uint32_t vni;
} list_tep_request;

typedef struct {
  command_reply_header header;
} add_tep_reply;

typedef struct {
  command_reply_header header;
} set_tep_reply;

typedef struct {
  command_reply_header header;
} del_tep_reply;

typedef struct {
  command_reply_header header;
  tunnel_endpoint tep[ 0 ];
} list_tep_reply;


#endif // REFLECTOR_CTRL_COMMON_H


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */


