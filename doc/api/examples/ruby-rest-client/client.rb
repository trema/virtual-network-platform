#
# Copyright (C) 2013 NEC Corporation
#

require 'rubygems'
require 'rest_client'
require 'json'

BASE_URL = "http://127.0.0.1:8081/networks"

def list_network
  url = BASE_URL
  get url
end

def create_network data
  url = BASE_URL
  post url, data
end

def show_network data
  url = "#{ BASE_URL }/%u" % data[ :id ]
  get url
end

def modify_network data
  url = "#{ BASE_URL }/%u" % data[ :id ]
  data.delete( :id )
  put url, data
end

def destroy_network data
  url = "#{ BASE_URL }/%u" % data[ :id ]
  delete url
end

def list_ports data
  url = "#{ BASE_URL }/%u/ports" % [ data[ :net_id ] ]
  get url
end

def create_port data
  url = "#{ BASE_URL }/%u/ports" % data[ :net_id ]
  data.delete( :net_id )
  post url, data
end

def delete_port data
  url = "#{ BASE_URL }/%u/ports/%u" % [ data[ :net_id ], data[ :id ] ]
  delete url
end

def show_port data
  url = "#{ BASE_URL }/%u/ports/%u" % [ data[ :net_id ], data[ :id ] ]
  get url
end

def list_macs data
  url = "#{ BASE_URL }/%u/ports/%u/mac_addresses" % [ data[ :net_id ], data[ :port_id ] ]
  data.delete( :net_id )
  data.delete( :port_id )
  get url
end

def add_mac data
  url = "#{ BASE_URL }/%u/ports/%u/mac_addresses" % [ data[ :net_id ], data[ :port_id ] ]
  data.delete( :net_id )
  data.delete( :port_id )
  post url, data
end

def delete_mac data
  url = "#{ BASE_URL }/%u/ports/%u/mac_addresses/%s" % [ data[ :net_id ], data[ :port_id ], data[ :address ] ]
  data.delete( :net_id )
  data.delete( :id )
  data.delete( :address )
  delete url
end

def show_mac data
  url = "#{ BASE_URL }/%u/ports/%u/mac_addresses/%s" % [ data[ :net_id ], data[ :port_id ], data[ :address ] ]
  data.delete( :net_id )
  data.delete( :id )
  data.delete( :address )
  get url
end

def get url
  begin
    puts "GET " + url
    response = RestClient.get url, :accept => :json
    puts "\nResponse:"
    puts "Code: " + response.code.to_s
    puts response.body
    JSON.parse( response.body, :symbolize_names => true )
  rescue => e
    puts "\nException: " + e.message
    puts "Code: " + e.http_code.to_s if e.respond_to?( 'http_code' )
    puts e.http_body  if e.respond_to?( 'http_body' )
  end
end

def post url, data
  body = JSON.pretty_generate( data )
  begin
    puts "POST " + url
    puts body
    response = RestClient.post url, body, :content_type => :json, :accept => :json
    puts "\nResponse:"
    if not response.headers[ :location ].nil?
      puts "Location: " + response.headers[ :location ]
    end
    puts "Code: " + response.code.to_s
    puts response.body
    JSON.parse( response.body, :symbolize_names => true ) unless response.body.empty?
  rescue => e
    puts "\nException: " + e.message
    if e.respond_to?( 'http_code' )
      puts "Code: " + e.http_code.to_s
      busy_here = ( e.http_code == 486 )
    else
      busy_here = false
    end
    puts e.http_body  if e.respond_to?( 'http_body' )
    if busy_here
      puts "sleeping and retry"
      sleep 2
      retry
    end
  end
end

def put url, data
  body = JSON.pretty_generate( data )
  begin
    puts "PUT " + url
    puts body
    response = RestClient.put url, body, :content_type => :json, :accept => :json
    puts "\nResponse:"
    puts "Code: " + response.code.to_s
    puts response.body
  rescue => e
    puts "\nException: " + e.message
    if e.respond_to?( 'http_code' )
      puts "Code: " + e.http_code.to_s
      busy_here = ( e.http_code == 486 )
    else
      busy_here = false
    end
    puts e.http_body  if e.respond_to?( 'http_body' )
    if busy_here
      puts "sleeping and retry"
      sleep 2
      retry
    end
  end
end

def delete url
  begin
    puts "DELETE " + url
    response = RestClient.delete url
    puts "\nResponse:"
    puts "Code: " + response.code.to_s
    puts response.body
  rescue => e
    puts "\nException: " + e.message
    if e.respond_to?( 'http_code' )
      puts "Code: " + e.http_code.to_s
      busy_here = ( e.http_code == 486 )
    else
      busy_here = false
    end
    puts e.http_body  if e.respond_to?( 'http_body' )
    if busy_here
      puts "sleeping and retry"
      sleep 2
      retry
    end
  end
end
