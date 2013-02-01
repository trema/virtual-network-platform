#
# Copryright (C) 2012-2013 NEC Corporation
#

CC = gcc
INSTALL = install
CFLAGS = -std=gnu99 -D_GNU_SOURCE -g -fno-strict-aliasing -Wall \
         -Wextra -Wformat=2 -Wcast-qual -Wcast-align -Wwrite-strings \
         -Wconversion -Wfloat-equal -Wpointer-arith -Werror
LDFLAGS = -pthread -lrt

VXLAND = vxland
VXLAND_SRCS = vxland.c fdb.c hash.c linked_list.c iftap.c net.c \
              vxlan_instance.c vxlan.c daemon.c log.c ctrl_if.c \
              vxlan_ctrl_server.c wrapper.c
VXLAND_OBJS = $(VXLAND_SRCS:.c=.o)

VXLANCTL = vxlanctl
VXLANCTL_SRCS = vxlanctl.c log.c ctrl_if.c vxlan_ctrl_client.c wrapper.c
VXLANCTL_OBJS = $(VXLANCTL_SRCS:.c=.o)

REFLECTORD = reflectord
REFLECTORD_SRCS = reflectord.c reflector_common.c receiver.c distributor.c \
                  ethdev.c log.c queue.c linked_list.c hash.c ctrl_if.c \
                  reflector_ctrl_server.c daemon.c vxlan.c wrapper.c
REFLECTORD_OBJS = $(REFLECTORD_SRCS:.c=.o)

REFLECTORCTL = reflectorctl
REFLECTORCTL_SRCS = reflectorctl.c ctrl_if.c reflector_ctrl_client.c log.c \
                    wrapper.c
REFLECTORCTL_OBJS = $(REFLECTORCTL_SRCS:.c=.o)

SRCS = $(VXLAND_SRCS) $(VXLANCTL_SRCS) $(REFLECTORD_SRCS) $(REFLECTORCTL_SRCS)
OBJS = $(VXLAND_OBJS) $(VXLANCTL_OBJS) $(REFLECTORD_OBJS) $(REFLECTORCTL_OBJS)

TARGETS = $(VXLAND) $(VXLANCTL) $(REFLECTORD) $(REFLECTORCTL)

DEPENDS = .depends

ifndef DESTDIR
SBINDIR=/usr/sbin
else
SBINDIR=$(DESTDIR)/usr/sbin
endif

.PHONY : all clean depend

all: depend $(TARGETS)

$(VXLAND): $(VXLAND_OBJS)
	$(CC) $(VXLAND_OBJS) $(LDFLAGS) -o $@

$(VXLANCTL): $(VXLANCTL_OBJS)
	$(CC) $(VXLANCTL_OBJS) $(LDFLAGS) -o $@

$(REFLECTORD): $(REFLECTORD_OBJS)
	$(CC) $(REFLECTORD_OBJS) $(LDFLAGS) -o $@

$(REFLECTORCTL): $(REFLECTORCTL_OBJS)
	$(CC) $(REFLECTORCTL_OBJS) $(LDFLAGS) -o $@

.c.o:
	$(CC) $(CFLAGS) -c $<

install: $(VXLAND) $(VXLANCTL) $(REFLECTORD) $(REFLECTORCTL)
	$(INSTALL) -o root -g root -D -m 755 $(VXLAND) $(SBINDIR)/$(VXLAND)
	$(INSTALL) -o root -g root -D -m 755 $(REFLECTORD) $(SBINDIR)/$(REFLECTORD)
	$(INSTALL) -o root -g root -D -m 755 $(VXLANCTL) $(SBINDIR)/$(VXLANCTL)
	$(INSTALL) -o root -g root -D -m 755 $(REFLECTORCTL) $(SBINDIR)/$(REFLECTORCTL)

depend:
	$(CC) -MM $(CFLAGS) $(SRCS) > $(DEPENDS)

clean:
	@rm -rf $(DEPENDS) $(OBJS) $(TARGETS) *~

-include $(DEPENDS)
