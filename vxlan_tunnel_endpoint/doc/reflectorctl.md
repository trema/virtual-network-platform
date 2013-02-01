reflectorctl(1) -- VXLAN packet reflector configuration command
===============================================================

## SYNOPSIS

`reflectorctl` COMMAND [OPTION]...

## DESCRIPTION

This is the configuration command for reflectord(1).

## COMMANDS

  * `-a`, `--add_tep`:
    Add a tunnel endpoint.

  * `-d`, `--del_tep`:
    Delete a tunnel endpoint.

  * `-s`, `--set_tep`:
    Set tunnel endpoint parameters.

  * `-l`, `--list_tep`:
    List tunnel endpoints.

  * `-h`, `--help`:
    Show help and exit.

## OPTIONS

  * `-n`, `--vni`=VNI:
    Virtual Network Identifier.

  * `-i`, `--ip`=IPV4_ADDRESS:
    IP address.

  * `-p`, `--port`=UDP_PORT:
    Destination UDP port.

## AUTHOR

CHIBA Yasunobu &lt;y-chiba@bq.jp.nec.com&gt;

## COPYRIGHT

Copyright (C) 2012-2013 NEC Corporation. License GPLv2: GNU GPL version 2
&lt;http://gnu.org/licenses/gpl-2.0.html&gt;. This is free software: you are
free to change and redistribute it. There is NO WARRANTY, to the extent
permitted by law.

## SEE ALSO

vxland(1), vxlanctl(1), reflectorctl(1)
