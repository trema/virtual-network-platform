#
# Copryright (C) 2013 NEC Corporation
#

RONN = ronn

SRCS = vxland.md vxlanctl.md reflectord.md reflectorctl.md
OBJS = $(SRCS:.md=.1)

.PHONY: all clean

.SUFFIXES: .md .1

all: $(OBJS)

.md.1:
	$(RONN) --roff $<

install:

clean:
	@rm -rf $(OBJS) *~
