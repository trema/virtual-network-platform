vxlanctl(1) -- VXLAN Tunnel End Point Utility
=============================================

## SYNOPSIS

`vxlanctl` -a -n VNI [ -i IPV4_ADDRESS ] [ -p UDP_PORT ] [ -t SECONDS ]

`vxlanctl` -s -n VNI [ -i IPV4_ADDRESS ] [ -p UDP_PORT ] [ -t SECONDS ]

`vxlanctl` -w -n VNI

`vxlanctl` -o -n VNI

`vxlanctl` -d -n VNI

`vxlanctl` -g [-q]

`vxlanctl` -l [-g] [-n VNI] [-q]

`vxlanctl` -f -n VNI

`vxlanctl` -e -n VNI -m MAC_ADDRESS -i IPV4_ADDRESS [-t AGING_TIME]

`vxlanctl` -b -n VNI [-m MAC_ADDRESS]

`vxlanctl` -h

## DESCRIPTION

The `vxlanctl` program is a configuration utility for vxland(1).

One of the following commands is a mandatory argument.

  * `-a`, `--add_instance`:
    Request to add a virtual network instance.
    If `-i` option is omitted, an IP address is inherited from the
    global configuration. If a multicast address is specified with
    `-i` option, IGMP membership report is sent out. A single
    multicast address may be shared by multiple virtual network
    instances.
    If `-p` option is omitted, a port is inherited from the global
    configuration.
    If `-t` option is omitted, a value is inherited from the global
    configuration.

  * `-s`, `--set_instance`:
    Request to change one or more parameters related to a virtual
    network instance.

  * `-w`, `--inactivate_instance`:
    Request to temporarily inactivate a virtual network instance.

  * `-o`, `--activate_instance`:
    Request to activate a virtual network instance.

  * `-d`, `--del_instance`:
    Request to delete a virtual network instance.

  * `-g`, `--show_global`:
    Show global configuration parameters.

  * `-l`, `--list_instances`:
    Request to list all virtual network instances and configuration
    parameters.

  * `-f`, `--show_fdb`:
    Request to show forwarding database entries for a specific
    virtual network instance.

  * `-e`, `--add_fdb_entry`:
    Request to add a forwarding database entry. Manually installed
    forwarding database entries override dynamic entries.
    If `-t` option is omitted, 0 is chosen as a aging time value.
    0 means the entry is never aged out.

  * `-b`, `--delete_fdb_entry`:
    Request to delete forwarding database entries.
    If `-m` option is omitted, all forwarding database entries are
    deleted.

  * `-h`, `--help`:
    Show help and exit.

The followings are options for commands above.

  * `-n`, `--vni`=VNI:
    Specify a virtual network identifier (VNI) in decimal or
    hexadecimal.

  * `-i`, `--ip`=IPV4_ADDRESS:
    When adding or setting virtual network instance with `-a` or `-s`
    command, specify a destination IPv4 address for sending packets
    which require flooding (broadcast/multicast/unknown unicast). Any
    multicast/unicast address can be specified.
    When adding a static forwarding database entry with `-e` command,
    specify a destination IPv4 unicast address for sending VXLAN
    packets for the target.

  * `-p`, `--port`=UDP_PORT:
    When adding or updating virtual network instance with `-a` or `-s`
    command, specify a destination UDP port for sending packets which
    require flooding.

  * `-m`, `--mac`=MAC_ADDRESS:
    Specify a target MAC address in XX:XX:XX:XX:XX:XX format.

  * `-t`, `--aging_time`=SECONDS:
    Specify a maximum aging time value (0 - 86400 seconds) for entries
    in the forwarding database in decimal. 0 means entries are never
    aged out.

  * `-q`, `--quiet`:
    Don't output header part of command output.

## EXIT STATUS

  * 0: Succeeded.
  * 1: Invalid parameter.
  * 5: Duplicated instance found.
  * 6: Specified instance not found.
  * 255: Any other error.

## AUTHOR

CHIBA Yasunobu &lt;y-chiba@bq.jp.nec.com&gt;

## COPYRIGHT

Copyright (C) 2012-2013 NEC Corporation. License GPLv2: GNU GPL version 2
&lt;http://gnu.org/licenses/gpl-2.0.html&gt;. This is free software: you are
free to change and redistribute it. There is NO WARRANTY, to the extent
permitted by law.

## SEE ALSO

vxland(1), vxlanctl(1), reflectord(1)
