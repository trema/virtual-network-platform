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
      get_configure
    end

    def shutdown
      # nop
    end

    def list_endpoints parameters
      raise BadReuestError.new "Vni must be specified." if parameters[ :vni ].nil?

      vni = convert_vni parameters[ :vni ]

      logger.debug "#{ __FILE__ }:#{ __LINE__ }: list of tunnel endpoints (vni = #{ vni })"

      tunnel_endpoint.list vni
    end

    def add_endpoint parameters
      raise BadReuestError.new "Vni must be specified." if parameters[ :vni ].nil?
      raise BadReuestError.new "IP address must be specified." if parameters[ :ip ].nil?
      raise BadReuestError.new "Port number must be specified." if parameters[ :port ].nil?

      vni = convert_vni parameters[ :vni ]
      address = convert_address parameters[ :ip ]
      port = convert_port parameters[ :port ]

      logger.debug "#{ __FILE__ }:#{ __LINE__ }: add a new tunnel endpoint (vni = #{ vni }, address = #{ address }, port = #{ port })"

      tunnel_endpoint.add vni, address, port
    end

    def delete_endpoint parameters
      raise BadReuestError.new "Vni must be specified." if parameters[ :vni ].nil?
      raise BadReuestError.new "IP address must be specified." if parameters[ :ip ].nil?

      vni = convert_vni parameters[ :vni ]
      address = convert_address parameters[ :ip ]

      logger.debug "#{ __FILE__ }:#{ __LINE__ }: delete the tunnel endpoint (vni = #{ vni }, address = #{ address })"

      tunnel_endpoint.delete vni, address
    end

    private

    def get_configure

      logger.debug "#{ __FILE__ }:#{ __LINE__ }: get configure #{ url }"

      response = RestClient.get url, :content_type => :json, :accept => :json, :timeout => 10, :open_timeout => 10, :user_agent => "reflector_agent"
      logger.debug "response #{ response.code }"
      logger.debug "#{ response.body }"
      raise "Invalid responce code (#{ response.code })" unless response.code == 200
      tunnel_endpoints = JSON.parse( response.body, :symbolize_names => true )
      if tunnel_endpoints.empty?
        return
      end
      tunnel_endpoints.each_pair do | vni, teps |
        vni = convert_vni vni.to_s
        teps.each do | tep |
          raise BadReuestError.new "IP address must be specified." if tep[ :ip ].nil?
          raise BadReuestError.new "Port number must be specified." if tep[ :port ].nil?

          address = convert_address tep[ :ip ]
          port = convert_port tep[ :port ]
          if port == 0
            port = nil
          end
          logger.debug "vni = #{ vni } address = #{ address } port = #{ port }"

          tunnel_endpoint.add vni, address, port
        end
      end
    end

    def logger
      Log.instance
    end

    def config
      Configure.instance
    end

    def url
      config.controller_uri + "reflector/config"
    end

    def tunnel_endpoint
      Vxlan::Reflector::TunnelEndpoint
    end

  end

end
