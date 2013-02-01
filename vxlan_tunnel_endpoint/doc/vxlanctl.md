vxlanctl(1) -- VXLAN tunnel endpoint configuration command
==========================================================

## SYNOPSIS

`vxlanctl` COMMAND [OPTION]...

## DESCRIPTION

This is the configuration command for vxland(1).

## COMMANDS

  * `-a`, `--add_instance`:
    Add a VXLAN instance.

  * `-s`, `--set_instance`:
    Set VXLAN parameters.

  * `-w`, `--inactivate_instance`:
    Inactivate a VXLAN instance.

  * `-o`, `--activate_instance`:
    Activate a VXLAN instance.

  * `-d`, `--del_instance`:
    Delete a VXLAN instance.

  * `-g`, `--show_global`:
    Show global settings.

  * `-l`, `--list_instances`:
    List all VXLAN instances.

  * `-f`, `--show_fdb`:
    Show forwarding database.

  * `-e`, `--add_fdb_entry`:
    Add a static forwarding database entry.

  * `-b`, `--delete_fdb_entry`:
    Delete a static forwarding database entry.

  * `-h`, `--help`:
    Show help and exit.

## OPTIONS

  * `-n`, `--vni`=VNI:
    Virtual Network Identifier.

  * `-i`, `--ip`=IPV4_ADDRESS:
    IP address.

  * `-p`, `--port`=UDP_PORT:
    UDP port.

  * `-m`, `--mac`=MAC_ADDRESS:
    MAC address.

  * `-t`, `--aging_time`=SECONDS:
    Aging time in seconds.

  * `-q`, `--quiet`:
    Don't output header part of command output.

## AUTHOR

CHIBA Yasunobu &lt;y-chiba@bq.jp.nec.com&gt;

## COPYRIGHT

Copyright (C) 2012-2013 NEC Corporation. License GPLv2: GNU GPL version 2
&lt;http://gnu.org/licenses/gpl-2.0.html&gt;. This is free software: you are
free to change and redistribute it. There is NO WARRANTY, to the extent
permitted by law.

## SEE ALSO

vxland(1), reflectord(1), reflectorctl(1)
