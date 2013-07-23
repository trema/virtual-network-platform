#!/bin/sh -

VERBOSE=-v

# show network

curl $VERBOSE \
        -H "Accept: application/json" \
        -H "Content-type: application/json" \
        -X GET \
        http://127.0.0.1:8081/networks/123
