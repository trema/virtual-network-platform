#
# Copyright (C) 2013 NEC Corporation
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 2, as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
#

begin
require 'rubygems'
rescue LoadError
end
require 'restclient'
require 'configure'
require 'convert'
require 'log'
require 'ovs'
require 'vxlan'

class OverlayNetwork
  class << self
    @registered = false

    def startup
      body = JSON.pretty_generate( {  :control_uri => config.uri, :tunnel_endpoint => config[ 'tunnel_endpoint' ] } )
      logger.debug "register #{ register_url }"
      logger.debug "#{ body }"
      begin
        response = RestClient.post register_url, body, :content_type => :json, :accept => :json, :timeout => 10, :open_timeout => 10, :user_agent => "virtual-network-agent"
      rescue => e
        raise e.exception "#{ e.message } (register url = '#{ register_url }')"
      end
      logger.debug "response #{ response.code }"
      logger.debug "#{ response.body }"
      raise "Invalid responce code (#{ response.code })" unless response.code == 202
      @registered = true
      logger.debug "registered"
    end

    def shutdown
      unless @registered
        logger.debug "not registered"
        return
      end
      logger.debug "register #{ unregister_url }"
      begin
        response = RestClient.delete unregister_url, :accept => :json, :timeout => 10, :open_timeout => 10, :user_agent => "virtual-network-agent"
      rescue => e
        raise e.exception "#{ e.message } (unregister url = '#{ register_url }')"
      end
      logger.debug "response #{ response.code }"
      logger.debug "#{ response.body }"
      raise "Invalid responce code (#{ response.code })" unless response.code == 202
    end

    def list parameters = {}
      ovs_ports = OVS::Port.list
      instances = {}
      Vxlan::Instance.list.each_pair do | vni, value |
        port_name = Vxlan::Instance.name( vni )
        if ovs_ports.index( port_name )
          instances[ vni ] = value
        end
      end
      instances
    end

    def create parameters
      raise BadReuestError.new "Vni must be specified." if parameters[ :vni ].nil?
      raise BadReuestError.new "Broadcast address must be specified." if parameters[ :broadcast ].nil?

      vni = convert_vni parameters[ :vni ]
      broadcast_address = convert_broadcast_address parameters[ :broadcast ]

      port_name = Vxlan::Instance.name( vni )

      if OVS::Port.exists?( port_name )
        raise DuplicatedOverlayNetwork.new vni
      end
      if Vxlan::Instance.exists?( vni )
        Vxlan::Instance.delete( vni )
      end
      begin
        Vxlan::Instance.add( vni, broadcast_address )
        OVS::Port.add port_name
      rescue =>e
        raise NetworkAgentError.new e.message
      end
    end

    def destroy parameters
      raise BadReuestError.new "Vni must be specified." if parameters[ :vni ].nil?

      vni = convert_vni parameters[ :vni ]

      port_name = Vxlan::Instance.name( vni )

      unless OVS::Port.exists?( port_name )
        raise NoOverlayNetworkFound.new vni
      end
      begin
        OVS::Port.delete port_name
        if Vxlan::Instance.exists?( vni )
          Vxlan::Instance.delete( vni )
        end
      rescue =>e
        raise NetworkAgentError.new e.message
      end
    end

    private

    def logger
      Log.instance
    end

    def config
      Configure.instance
    end

    def register_url
      datapath_id = OVS::Bridge.datapath_id
      config.controller_uri + "agents/" + datapath_id.to_s
    end
    alias :unregister_url :register_url

  end

end
