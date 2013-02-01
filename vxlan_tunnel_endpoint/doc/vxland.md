vxland(1) -- VXLAN tunnel endpoint daemon
=========================================

## SYNOPSIS

`vxland` [OPTION]...

## DESCRIPTION

VXLAN tunnel endpoint implementation on Linux userspace.

## OPTIONS

  * `-i`, `--interface`=INTERFACE:
    Set network interface for sending/receiving VXLAN packets.

  * `-p`, `--port`=UDP_PORT:
    Set UDP port for receiving VXLAN packets.

  * `-a`, `--flooding_address`=IPV4_ADDR:
    Set default destination IP address for sending flooding packets.

  * `-f`, `--flooding_port`=UDP_PORT:
    Set default destination UDP port for sending flooding packets.

  * `-t`, `--aging_time`=SECONDS:
    Set default aging time in seconds. This value may be overridden
    by individual configuration done by vxlanctl(1).

  * `-s`, `--syslog`:
    Output log messages to syslog.

  * `-d`, `--daemonize`:
    Daemonize.

  * `-h`, `--help`:
    Show help and exit.

## AUTHOR

CHIBA Yasunobu &lt;y-chiba@bq.jp.nec.com&gt;

## COPYRIGHT

Copyright (C) 2012-2013 NEC Corporation. License GPLv2: GNU GPL version 2
&lt;http://gnu.org/licenses/gpl-2.0.html&gt;. This is free software: you are
free to change and redistribute it. There is NO WARRANTY, to the extent
permitted by law.

## SEE ALSO

vxlanctl(1), reflectord(1), reflectorctl(1)

