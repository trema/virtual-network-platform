#!/bin/sh -

VERBOSE=-v
WAIT='sleep 5'

curl $VERBOSE \
        -H "Accept: application/json" \
        -H "Content-type: application/json" \
        -X DELETE \
        http://127.0.0.1:8081/networks/123/ports/3/mac_addresses/00:00:00:00:00:02

$WAIT

curl $VERBOSE \
        -H "Accept: application/json" \
        -H "Content-type: application/json" \
        -X DELETE \
        http://127.0.0.1:8081/networks/123/ports/3

$WAIT

curl $VERBOSE \
        -H "Accept: application/json" \
        -H "Content-type: application/json" \
        -X DELETE \
        http://127.0.0.1:8081/networks/123/ports/1/mac_addresses/00:00:00:00:00:01

$WAIT

curl $VERBOSE \
        -H "Accept: application/json" \
        -H "Content-type: application/json" \
        -X DELETE \
        http://127.0.0.1:8081/networks/123/ports/1

$WAIT

# destroy network

curl $VERBOSE \
        -H "Accept: application/json" \
        -H "Content-type: application/json" \
        -X DELETE \
        http://127.0.0.1:8081/networks/123
