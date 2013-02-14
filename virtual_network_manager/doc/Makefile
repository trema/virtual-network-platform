#
# Copyright (C) 2012-2013 NEC Corporation
#
#
# Installing required packages (Ubuntu 12.04)
#   $ sudo apt-get install graphviz mscgen librsvg2-bin
#

DOT = dot
DOT_OPTS = -Tpdf
MSCGEN = mscgen
MSCGEN_OPTS = -Tsvg
RSVG_CONVERT = rsvg-convert
RSVG_CONVERT_OPTS = -f pdf
SRCS_FSM = slice_fsm.dot port_fsm.dot mac_fsm.dot
SRCS_MSC = openflow_initial.msc create_slice.msc modify_slice.msc \
           destroy_slice.msc add_port_slice_join.msc \
           add_port_slice_exist.msc add_port_error.msc \
           add_port_timeout.msc delete_port_slice_remain.msc \
           delete_port_slice_leave.msc delete_port_error.msc \
           delete_port_timeout.msc add_mac.msc delete_mac.msc
OBJS = $(SRCS_FSM:.dot=.pdf) $(SRCS_MSC:.msc=.pdf)

.PHONY: all clean

.SUFFIXES: .dot .msc .svg .pdf

all: $(OBJS)

.dot.pdf:
	$(DOT) $(DOT_OPTS) -o $@ $<

.msc.svg:
	$(MSCGEN) $(MSCGEN_OPTS) -o $@ $<

.svg.pdf:
	$(RSVG_CONVERT) $(RSVG_CONVERT_OPTS) -o $@ $<

clean:
	@rm -rf $(OBJS) *~
