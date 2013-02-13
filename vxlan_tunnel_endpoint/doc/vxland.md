vxland(1) -- VXLAN Tunnel End Point
===================================

## SYNOPSIS

`vxland` -i INTERFACE [OPTION]...

## DESCRIPTION

The `vxland` program is a Virtual eXtensible Local Area Network
(VXLAN) Tunnel End Point (VTEP) implementation on Linux userspace.

A network interface for sending/receiving VXLAN packets must be
specified with `-i` option.

  * `-i`, `--interface`=INTERFACE:
    Specify a network interface for sending/receiving VXLAN packets.
    The network interface may or may not have an IP address on startup.
    IP address may be assigned, changed, or revoked in operation.

The following options are not mandatory options.

  * `-p`, `--port`=UDP_PORT:
    Specify a local UDP port for receiving VXLAN packets in decimal.
    If omitted, 60000 is chosen by default.

  * `-a`, `--flooding_address`=IPV4_ADDR:
    Specify a default destination IPv4 address for sending packets
    which require flooding. If omitted, 239.0.0.1 is chosen by default.

  * `-f`, `--flooding_port`=UDP_PORT:
    Specify a default destination UDP port for sending packets which
    require flooding. If omitted, a port specified with '-p' option is
    chosen.

  * `-t`, `--aging_time`=SECONDS:
    Specify a default maximum aging time value (0 - 86400 seconds) for
    entries in the forwarding database in decimal. 0 means entries are
    never aged out. If omitted, default value (300) is chosen.

  * `-s`, `--syslog`:
    Output log messages to syslog. By default, log messages are shown on
    stdout/stderr.

  * `-d`, `--daemonize`:
    Daemonize. By default, VXLAN service/daemon runs in the foreground.

  * `-h`, `--help`:
    Show help and exit.

## EXIT STATUS

  * 0: Succeeded.
  * 1: Invalid parameter.
  * 2: Another VXLAN daemon is running.
  * 3: Port already in use.
  * 4: Requested source ports cannot be allocated.
  * 255: Any other error.

## AUTHOR

CHIBA Yasunobu &lt;y-chiba@bq.jp.nec.com&gt;

## COPYRIGHT

Copyright (C) 2012-2013 NEC Corporation. License GPLv2: GNU GPL version 2
&lt;http://gnu.org/licenses/gpl-2.0.html&gt;. This is free software: you are
free to change and redistribute it. There is NO WARRANTY, to the extent
permitted by law.

## SEE ALSO

vxlanctl(1), reflectord(1), reflectorctl(1)

