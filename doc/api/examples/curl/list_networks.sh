#!/bin/sh -

VERBOSE=-v

# list networks

curl $VERBOSE \
        -H "Accept: application/json" \
        -H "Content-type: application/json" \
        -X GET \
        http://127.0.0.1:8081/networks
