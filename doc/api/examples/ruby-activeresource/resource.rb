#
# Copyright (C) 2013 NEC Corporation
#

require 'rubygems'
require 'active_resource'

module ActiveResource
  # 486 Busy Here
  class BusyHereError < ClientError
  end
end

class Base < ActiveResource::Base
  self.site = 'http://127.0.0.1:8081/'
  self.format = :json
  self.timeout = 1

  def self.element_path( id, prefix_options = {}, query_options = nil )
    super
    "#{ prefix( prefix_options ) }#{ collection_name }/#{ id }"
  end

  def self.new_element_path( prefix_options = {} )
    super
    "#{ prefix( prefix_options ) }#{ collection_name }"
  end

  def self.collection_path( prefix_options = {}, query_options = nil )
    super
    "#{ prefix( prefix_options ) }#{ collection_name }"
  end

  def self.delete( id, options = {} )
    check_busy( options ) do
      super
    end
  end

  def save( options = {} )
    Base.check_busy( options ) do
      super()
    end
  end

  def save!( options = {} )
    retry_count = 1
    begin
      save || raise( ActiveResource::ResourceInvalid.new( @remote_errors.response ) )
    rescue ActiveResource::BusyHereError
      raise if retry_count >= 5
      # sleeping and retry
      retry_count += 1
      sleep 2
      retry
    end
  end

  def destroy( options = {} )
    Base.check_busy( options ) do
      super()
    end
  end

  private

  def load_remote_errors(remote_errors, save_cache = false )
    errors[ :base ] << remote_errors.response.body
  end

  def self.check_busy( options = {} )
    retry_count = ( retry_count || 0 ) + 1
    yield
  rescue ActiveResource::ClientError => e
    if e.response.code.to_i == 486
      if options[ :retry_on_busy ] && retry_count < 5
        # sleeping and retry
        sleep 2
        retry
      end
      raise ActiveResource::BusyHereError.new( e.response )
    else
      raise
    end
  end

end

class Network < Base
  self.prefix = '/'

  def to_json( options = {} )
    params = {}
    if new?
      params[ 'id' ] = attributes[ 'id' ] unless attributes[ 'id' ].nil?
    end
    params[ 'description' ] = attributes[ 'description' ] unless attributes[ 'description' ].nil?
    connection.format.encode( params )
  end

end

class Port < Base
  self.prefix = '/networks/:net_id/'

  def to_json( options = {} )
    params = {}
    if new?
      params[ 'id' ] = attributes[ 'id' ] unless attributes[ 'id' ].nil?
      params[ 'datapath_id' ] = attributes[ 'datapath_id' ]
      params[ 'name' ] = attributes[ 'name' ]
      params[ 'vid' ] = attributes[ 'vid' ]
      params[ 'description' ] = attributes[ 'description' ] unless attributes[ 'description' ].nil?
    else
      raise 'Port update is not supported'
    end
    connection.format.encode( params )
  end

end

class MacAddress < Base
  self.prefix = '/networks/:net_id/ports/:port_id/'
  self.primary_key = :address

  def to_json( options = {} )
    params = {}
    if new?
      params[ 'address' ] = attributes[ 'address' ]
    else
      raise 'MacAddress update is not supported'
    end
    connection.format.encode( params )
  end

end
