#
# Copyright (C) 2012 NEC Corporation
# NEC Confidential
#

CC = gcc

ifneq ($(wildcard /etc/redhat-release),) 
	TREMA_CONFIG = trema-config
	OVS_CFLAGS = -I/usr/include/trema/ovs
	OVS_LDFLAGS = -L/usr/lib -L/usr/lib64 -ltrema_ovs
else
	TREMA_CONFIG = ../../../trema/trema-config
	OVS_CFLAGS = -I../../ovs-extensions-support-for-trema
	OVS_LDFLAGS = -L../../ovs-extensions-support-for-trema -ltrema_ovs
endif

CFLAGS = $(shell $(TREMA_CONFIG) --cflags) $(shell mysql_config --include) $(OVS_CFLAGS) -g -std=gnu99 -D_GNU_SOURCE -fno-strict-aliasing -Werror -Wall -Wextra -Wformat=2 -Wcast-qual -Wcast-align -Wwrite-strings -Wconversion -Wfloat-equal -Wpointer-arith
LDFLAGS = $(shell $(TREMA_CONFIG) --libs) $(shell mysql_config --libs_r) $(shell curl-config --libs) $(OVS_LDFLAGS) -ljson

TARGET = bisco_vnet_manager
SRCS = vnet_manager.c slice.c transaction_manager.c switch.c db.c http_client.c queue.c overlay_network_manager.c utilities.c
OBJS = $(SRCS:.c=.o)

DEPENDS = .depends

.PHONY: all clean

.SUFFIXES: .c .o

all: depend $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) -o $@

.c.o:
	$(CC) $(CFLAGS) -c $<

depend: 
	$(CC) -MM $(CFLAGS) $(SRCS) > $(DEPENDS)

clean:
	@rm -rf $(DEPENDS) $(OBJS) $(TARGET) *~

-include $(DEPENDS)
