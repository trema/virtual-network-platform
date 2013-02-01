reflectord(1) -- VXLAN packet reflector
=======================================

## SYNOPSIS

`reflectord` -i INTERFACE [OPTION]...

## DESCRIPTION

VXLAN packet reflector which allows you to deploy VXLAN-based virtual
networks without multicast capable underlay network.

## OPTIONS

  * `-i`, `--interface`=INTERFACE:
    Set network interface for sending/receiving VXLAN packets.

  * `-p`, `--port`=UDP_PORT:
    Set UDP port for receiving VXLAN packets

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

vxlanctl(1), vxland(1), reflectorctl(1)

