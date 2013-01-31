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

require 'configure'
require 'log'
require 'ovs'
require 'vxlan'

class OverlayNetwork
  class << self
    @registered = false

    def startup
      body = JSON.pretty_generate( {  :control_uri => config[ 'uri' ], :tunnel_endpoint => config[ 'tunnel_endpoint' ] } )
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

    def create parameters
      raise BadReuestError.new "Vni must be specified." if parameters[ :vni ].nil?
      raise BadReuestError.new "Broadcast address must be specified." if parameters[ :broadcast ].nil?

      vni = convert_vni parameters[ :vni ]
      broadcast_address = convert_broadcast_address parameters[ :broadcast ]

      if vxlan_instance.exists?( vni )
	raise DuplicatedOverlayNetwork.new vni
      end
      begin
        vxlan_instance.add( vni, broadcast_address )
	port_name = vxlan_instance.name( vni )
        ovs_port.add port_name
      rescue =>e
        raise NetworkAgentError.new e.message
      end
    end

    def destroy parameters
      raise BadReuestError.new "Vni must be specified." if parameters[ :vni ].nil?

      vni = convert_vni parameters[ :vni ]

      unless vxlan_instance.exists?( vni )
	raise NoOverlayNetworkFound.new vni
      end
      begin
	port_name = vxlan_instance.name( vni )
        ovs_port.delete port_name
        vxlan_instance.delete( vni )
      rescue =>e
        raise NetworkAgentError.new e.message
      end
    end

    private

    def ovs_port
      OVS::Port
    end

    def vxlan_instance
      Vxlan::Instance
    end

    def logger
      Log.instance
    end

    def config
      Configure.instance
    end

    def register_url
      config[ 'controller_uri' ] + "agents/" + config[ 'datapath_id' ].to_s
    end
    alias :unregister_url :register_url

  end

end
