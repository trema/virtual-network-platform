#! /bin/sh
#
# Copyright (C) 2012 NEC Corporation
#
# NEC Confidential
#

BASE_URI="http://127.0.0.1:8888/networks"

CLIENT="./httpc"

run(){
    method=$1
    target=$BASE_URI$2
    content=$3
    echo "#### BEGIN: $method $target ####"
    $CLIENT $method $target $content
    echo "#### END: $method $target ####"
    echo
}

tests(){
    run GET ""
    run POST "" create_network.json
    run PUT "/128" modify_network.json
    run GET "/128"
    run POST "/128/ports" create_port.json
    run GET "/128/ports"
    run GET "/128"
    run DELETE "/128/ports/0"
    run GET "/128/ports"
    run DELETE "/128"
    run GET ""
}

tests
