reflectorctl(1) -- VXLAN Packet Reflector Utility
=================================================

## SYNOPSIS

`reflectorctl` -a -n VNI -i IPV4_ADDRESS [ -p UDP_PORT ]

`reflectorctl` -d -n VNI -i IPV4_ADDRESS

`reflectorctl` -s -n VNI -i IPV4_ADDRESS -p UDP_PORT

`reflectorctl` -l [ -n VNI ]

`reflectorctl` -h

## DESCRIPTION

The `reflectorctl` program is a configuration utility for reflectord(1).

One of the following commands is a mandatory argument.

  * `-a`, `--add_tep`:
    Request to add a TEP for a specific virtual network instance.
    If `-p` option is omitted, destination port numbers are kept and
    not modified.

  * `-d`, `--del_tep`:
    Request to delete a TEP for a specific virtual network instance.

  * `-s`, `--set_tep`:
    Request to set parameters for a specific TEP.

  * `-l`, `--list_tep`:
    Request to show TEPs and configuration parameters.

  * `-h`, `--help`:
    Show help and exit.

The followings are options for commands above.

  * `-n`, `--vni`=VNI:
    Specify a virtual network identifier (VNI) in decimal or
    hexadecimal.

  * `-i`, `--ip`=IPV4_ADDRESS:
    Specify a destination IPv4 unicast address for sending VXLAN
    packets.

  * `-p`, `--port`=UDP_PORT:
    Specify a destination UDP port for sending VXLAN packets.

## EXIT STATUS

  * 0: Succeeded.
  * 1: Invalid parameter.
  * 4: Duplicated TEP entry found.
  * 5: TEP entry not found.
  * 255: Any other error.

## AUTHOR

CHIBA Yasunobu &lt;y-chiba@bq.jp.nec.com&gt;

## COPYRIGHT

Copyright (C) 2012-2013 NEC Corporation. License GPLv2: GNU GPL version 2
&lt;http://gnu.org/licenses/gpl-2.0.html&gt;. This is free software: you are
free to change and redistribute it. There is NO WARRANTY, to the extent
permitted by law.

## SEE ALSO

vxland(1), vxlanctl(1), reflectorctl(1)
