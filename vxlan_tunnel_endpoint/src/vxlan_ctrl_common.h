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


#ifndef VXLAN_CTRL_COMMON_H
#define VXLAN_CTRL_COMMON_H


#include <netinet/in.h>
#include "ctrl_if.h"
#include "fdb.h"
#include "vxlan_common.h"
#include "vxlan_instance.h"


#define CTRL_SERVER_SOCK_FILE "/tmp/.vxland"


enum {
  ADD_INSTANCE_REQUEST,
  ADD_INSTANCE_REPLY,
  SET_INSTANCE_REQUEST,
  SET_INSTANCE_REPLY,
  INACTIVATE_INSTANCE_REQUEST,
  INACTIVATE_INSTANCE_REPLY,
  ACTIVATE_INSTANCE_REQUEST,
  ACTIVATE_INSTANCE_REPLY,
  DEL_INSTANCE_REQUEST,
  DEL_INSTANCE_REPLY,
  SHOW_GLOBAL_REQUEST,
  SHOW_GLOBAL_REPLY,
  LIST_INSTANCES_REQUEST,
  LIST_INSTANCES_REPLY,
  SHOW_FDB_REQUEST,
  SHOW_FDB_REPLY,
  ADD_FDB_ENTRY_REQUEST,
  ADD_FDB_ENTRY_REPLY,
  DEL_FDB_ENTRY_REQUEST,
  DEL_FDB_ENTRY_REPLY,
  MESSAGE_TYPE_MAX,
};

enum {
  SET_VNI = 0x0001,
  SET_IP_ADDR = 0x0002,
  SET_UDP_PORT = 0x0004,
  SET_MAC_ADDR = 0x0008,
  SET_AGING_TIME = 0x0010,
  SHOW_GLOBAL = 0x0020,
  DISABLE_HEADER = 0x0040,
};


typedef struct {
  command_request_header header;
  struct vxlan_instance instance;
} add_instance_request;

typedef struct {
  command_request_header header;
  uint16_t set_bitmap;
  struct vxlan_instance instance;
} set_instance_request;

typedef struct {
  command_request_header header;
  uint32_t vni;
} del_instance_request;

typedef struct {
  command_request_header header;
} show_global_request;

typedef struct {
  command_request_header header;
  uint32_t vni;
} list_instances_request;

typedef struct {
  command_request_header header;
  uint32_t vni;
} show_fdb_request;

typedef show_fdb_request inactivate_instance_request;
typedef show_fdb_request activate_instance_request;

typedef struct {
  command_request_header header;
  uint32_t vni;
  struct ether_addr eth_addr;
  struct in_addr ip_addr;
  time_t aging_time;
} add_fdb_entry_request;

typedef struct {
  command_request_header header;
  uint32_t vni;
  struct ether_addr eth_addr;
} del_fdb_entry_request;

typedef struct {
  command_reply_header header;
} add_instance_reply;

typedef struct {
  command_reply_header header;
} set_instance_reply;

typedef struct {
  command_reply_header header;
} del_instance_reply;

typedef del_instance_reply inactivate_instance_reply;
typedef del_instance_reply activate_instance_reply;

typedef struct {
  command_reply_header header;
  struct vxlan vxlan[ 0 ];
} show_global_reply;

typedef struct {
  command_reply_header header;
  struct vxlan_instance instances[ 0 ];
} list_instances_reply;

typedef struct {
  command_reply_header header;
  struct fdb_entry entries[ 0 ];
} show_fdb_reply;

typedef del_instance_reply add_fdb_entry_reply;
typedef del_instance_reply del_fdb_entry_reply;


#endif // VXLAN_CTRL_COMMON_H


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
