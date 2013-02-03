reflectord(1) -- VXLAN Packet Reflector
=======================================

## SYNOPSIS

`reflectord` -i INTERFACE [OPTION]...

## DESCRIPTION

The `reflectord` program works with vland(1) and allows you to deploy
VXLAN-based virtual networks without multicast capable underlay
network.

A network interface for sending/receiving VXLAN packets must be
specified with `-i` option.

  * `-i`, `--interface`=INTERFACE:
    Specify a network interface for sending/receiving VXLAN packets.
    The network interface may or may not have an IP address on startup.
    IP address may be assigned, changed, or revoked in operation.

The following options are not mandatory arugments.

  * `-p`, `--port`=UDP_PORT:
    Specify a UDP port for receiving VXLAN packets in decimal.
    If omitted, default port number (60000) is chosen.

  * `-s`, `--syslog`:
    Output log messages to syslog.
    By default, log messages are shown on stdout/stderr.

  * `-d`, `--daemonize`:
    Daemonize. By default, Packet Reflector runs in the foreground.

  * `-h`, `--help`:
    Show help and exit.

## EXIT STATUS

  * 0: Succeeded.
  * 1: Invalid parameter.
  * 2: Another Packet Reflector is running.
  * 3: Port already in use.
  * 255: Any other error.

## AUTHOR

CHIBA Yasunobu &lt;y-chiba@bq.jp.nec.com&gt;

## COPYRIGHT

Copyright (C) 2012-2013 NEC Corporation. License GPLv2: GNU GPL version 2
&lt;http://gnu.org/licenses/gpl-2.0.html&gt;. This is free software: you are
free to change and redistribute it. There is NO WARRANTY, to the extent
permitted by law.

## SEE ALSO

vxlanctl(1), vxland(1), reflectorctl(1)

