#
# Copyright (C) 2012-2013 NEC Corporation
# NEC Confidential
#

CC = gcc
AR = ar
RANLIB = ranlib

TREMA_CONFIG = ../../trema/trema-config

CFLAGS = $(shell $(TREMA_CONFIG) --cflags) $(shell mysql_config --include) -g -std=gnu99 -D_GNU_SOURCE -fno-strict-aliasing -Werror -Wall -Wextra -Wformat=2 -Wcast-qual -Wcast-align -Wwrite-strings -Wconversion -Wfloat-equal -Wpointer-arith
LDFLAGS = $(shell $(TREMA_CONFIG) --libs) $(shell mysql_config --libs_r) $(shell curl-config --libs) -L. -ljson -lovs

TARGET = virtual_network_manager
SRCS = vnet_manager.c slice.c transaction_manager.c switch.c db.c http_client.c queue.c overlay_network_manager.c utilities.c
OBJS = $(SRCS:.c=.o)

LIB = libovs.a
LIB_SRCS = ovs_byteorder.c ovs_match.c ovs_actions.c ovs_utility.c ovs.c
LIB_OBJS = $(LIB_SRCS:.c=.o)

DEPENDS = .depends

.PHONY: all clean

.SUFFIXES: .c .o

all: depend $(TARGET)

$(TARGET): $(OBJS) $(LIB)
	$(CC) $(OBJS) $(LDFLAGS) -o $@

$(LIB): $(LIB_OBJS)
	@rm -f $(LIB)
	$(AR) -cq $(LIB) $(LIB_OBJS)
	$(RANLIB) $(LIB)

.c.o:
	$(CC) $(CFLAGS) -c $<

depend: 
	$(CC) -MM $(CFLAGS) $(SRCS) > $(DEPENDS)

clean:
	@rm -rf $(DEPENDS) $(OBJS) $(TARGET) $(LIB_OBJS) $(LIB) *~

-include $(DEPENDS)
