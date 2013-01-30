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


#ifndef CTRL_IF_H
#define CTRL_IF_H


#include <net/ethernet.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdint.h>


#define COMMAND_MESSAGE_LENGTH 4096


enum {
  STATUS_OK,
  STATUS_NG,
  STATUS_MAX,
};


enum {
  FLAG_NONE = 0x00,
  FLAG_MORE = 0x01,
};


typedef struct {
  uint32_t xid;
  uint8_t type;
  uint16_t length;
} command_request_header;

typedef struct {
  uint32_t xid;
  uint8_t type;
  uint8_t status;
  uint8_t reason;
  uint8_t flags;
  uint16_t length;
} command_reply_header;


extern volatile bool running;


ssize_t send_command( int fd, void *command, size_t *length );
ssize_t recv_command( int fd, void *command, size_t *length );
bool init_ctrl_client( int *fd );
bool finalize_ctrl_client( int fd );
bool init_ctrl_server( int *listen_fd, const char *file );
bool finalize_ctrl_server( int listen_fd, const char *file );


#endif // CTRL_IF_H


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */


