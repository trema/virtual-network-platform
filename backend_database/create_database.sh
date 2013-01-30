#! /bin/sh
#
# Copyright (C) 2012-2013 NEC Corporation
#

MYSQL="mysql"
MYSQL_USER="root"
MYSQL_PASSWORD="root123"
MYSQL_OPTS="--user=$MYSQL_USER --password=$MYSQL_PASSWORD"

$MYSQL $MYSQL_OPTS < ./create_database.sql
$MYSQL $MYSQL_OPTS < ./create_slice_table.sql
$MYSQL $MYSQL_OPTS < ./create_switch_table.sql
$MYSQL $MYSQL_OPTS < ./create_agent_table.sql
$MYSQL $MYSQL_OPTS < ./create_overlay_network_table.sql
$MYSQL $MYSQL_OPTS < ./create_status_table.sql
