#
# Copyright (C) 2012 NEC Corporation
# NEC Confidential
#

DOT = dot
DOT_OPTS = -Tpdf
MSCGEN = mscgen
MSCGEN_OPTS = -Teps
SRCS_FSM = slice_fsm.dot port_fsm.dot mac_fsm.dot
SRCS_MSC = openflow_initial.msc create_slice.msc modify_slice.msc destroy_slice.msc \
           add_port_slice_join.msc add_port_slice_exist.msc add_port_error.msc add_port_timeout.msc \
           delete_port_slice_remain.msc delete_port_slice_leave.msc delete_port_error.msc delete_port_timeout.msc \
           add_mac.msc delete_mac.msc
OBJS = $(SRCS_FSM:.dot=.pdf) $(SRCS_MSC:.msc=.eps)

.PHONY: all clean

.SUFFIXES: .dot .msc .eps .pdf

all: $(OBJS)

.dot.pdf:
	$(DOT) $(DOT_OPTS) -o $@ $<

.msc.eps:
	$(MSCGEN) $(MSCGEN_OPTS) -o $@ $<

clean:
	@rm -rf $(OBJS) *~

