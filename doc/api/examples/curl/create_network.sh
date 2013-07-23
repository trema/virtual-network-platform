#!/bin/sh -

VERBOSE=-v
WAIT='sleep 5'

# create network

curl $VERBOSE \
        -H "Accept: application/json" \
        -H "Content-type: application/json" \
        -X POST \
        -d '{ "id": 123, "description": "Virtual network #123" }' \
        http://127.0.0.1:8081/networks

$WAIT

# create port

curl $VERBOSE \
        -H "Accept: application/json" \
        -H "Content-type: application/json" \
        -X POST \
        -d '{ "id": 1, "datapath_id": "1", "name": "veth0-0",
        "vid": 65535, "description": "veth0-0 on switch #1" }' \
        http://127.0.0.1:8081/networks/123/ports

$WAIT

# add mac address

curl $VERBOSE \
        -H "Accept: application/json" \
        -H "Content-type: application/json" \
        -X POST \
        -d '{ "address" : "00:00:00:00:00:01" }' \
        http://127.0.0.1:8081/networks/123/ports/1/mac_addresses

$WAIT

# create port

curl $VERBOSE \
        -H "Accept: application/json" \
        -H "Content-type: application/json" \
        -X POST \
        -d '{ "id": 3, "datapath_id": "2", "name": "veth0-0",
        "vid": 65535, "description": "veth0-0 on switch #2" }' \
        http://127.0.0.1:8081/networks/123/ports

$WAIT

# add mac address

curl $VERBOSE \
        -H "Accept: application/json" \
        -H "Content-type: application/json" \
        -X POST \
        -d '{ "address" : "00:00:00:00:00:02" }' \
        http://127.0.0.1:8081/networks/123/ports/3/mac_addresses
