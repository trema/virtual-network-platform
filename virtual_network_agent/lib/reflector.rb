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

require 'open3'
begin
require 'rubygems'
rescue LoadError
end
require 'rest_client'
require 'json'

require 'configure'
require 'log'
require 'vxlan'

class Reflector
  class << self
    def startup
      logger.debug "get configure #{ url }"
      response = RestClient.get url, body, :content_type => :json, :accept => :json, :timeout => 10, :open_timeout => 10, :user_agent => "virtual-network-agent"
      logger.debug "response #{ response.code }"
      logger.debug "#{ response.body }"
      raise "Invalid responce code (#{ response.code })" unless response.code == 200
      tunnel_endpoints = JSON.parse( response.body, :symbolize_names => true )
      if tunnel_endpoints.empty?
        return
      end
      response do | vni, teps |
        vni = convert_vni vni
	teps.list.each do | tep |
          raise BadReuestError.new "IP address must be specified." if tep[ :ip ].nil?
          raise BadReuestError.new "Port number must be specified." if tep[ :port ].nil?

          address = convert_address tep[ :ip ]
          port = convert_port tep[ :port ]
	  logger.debug "vni = #{ vni } address = #{ address } port = #{ port }"

	  tunnel_endpoint.add vni, address, port
	end
      end
    end

    def shutdown
      # nop
    end

    def list_endpoints parameters
      raise BadReuestError.new "Vni must be specified." if parameters[ :vni ].nil?

      vni = convert_vni parameters[ :vni ]

      tunnel_endpoint.list vni
    end

    def add_endpoint parameters
      raise BadReuestError.new "Vni must be specified." if parameters[ :vni ].nil?
      raise BadReuestError.new "IP address must be specified." if parameters[ :ip ].nil?
      raise BadReuestError.new "Port number must be specified." if parameters[ :port ].nil?

      vni = convert_vni parameters[ :vni ]
      address = convert_address parameters[ :ip ]
      port = convert_port parameters[ :port ]

      tunnel_endpoint.add vni, address, port
    end

    def delete_endpoint parameters
      raise BadReuestError.new "Vni must be specified." if parameters[ :vni ].nil?
      raise BadReuestError.new "IP address must be specified." if parameters[ :ip ].nil?

      vni = convert_vni parameters[ :vni ]
      address = convert_address parameters[ :ip ]

      tunnel_endpoint.delete vni, address
    end

    def logger
      Log.instance
    end

    def config
      Configure.instance
    end

    def url
      config.controller_uri + "config"
    end

    def tunnel_endpoint
      Vxlan::Reflector::TunnelEndpoint
    end

  end

end
