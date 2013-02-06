#! /bin/sh
#
# Copyright (C) 2012-2013 NEC Corporation
#

BASE_URI="http://127.0.0.1:8081/networks"

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
	    -X $method -d "$content" $target
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
    run PUT "/2" modify_network.json
    run GET "/2"
    run POST "/2/ports" create_port.json
    run GET "/2/ports"
    run GET "/2"
    sleep 3
    run DELETE "/2/ports/3"
    run GET "/2/ports"
    sleep 3
    run DELETE "/2"
    run GET ""
}

tests
