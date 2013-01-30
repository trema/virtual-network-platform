#! /bin/sh
#
# Copyright (C) 2012-2013 NEC Corporation
#

BASE_URI="http://127.0.0.1:8888/networks"

CURL="curl"

run(){
    local method=$1
    local target=$BASE_URI$2
    local file=$3
    local content

    if [ -n "$file" ]; then
	content=`cat $file`
    fi

    echo "#### BEGIN: $method $target ####"
    if [ -n "$content" ]; then
	$CURL -v -H "Accept: application/json" \
	    -H "Content-type: application/json" \
	    -X $method -d "'$content'" $target
	echo
    else
	$CURL -v -H "Accept: application/json" -X $method $target
    fi
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
