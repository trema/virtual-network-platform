#!/bin/sh -

VERBOSE=-v

# list ports

curl $VERBOSE \
        -H "Accept: application/json" \
        -H "Content-type: application/json" \
        -X GET \
        http://127.0.0.1:8081/networks/123/ports

# list mac addresses

curl $VERBOSE \
        -H "Accept: application/json" \
        -H "Content-type: application/json" \
        -X GET \
        http://127.0.0.1:8081/networks/123/ports/1/mac_addresses

# list mac addresses

curl $VERBOSE \
        -H "Accept: application/json" \
        -H "Content-type: application/json" \
        -X GET \
        http://127.0.0.1:8081/networks/123/ports/3/mac_addresses
