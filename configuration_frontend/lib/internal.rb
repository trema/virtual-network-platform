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

require 'errors'
require 'db'
require 'convert'
require 'log'

module Internal

  class Switch
    class << self
      def list parameters = {}
        DB::Switch.find( :all, :readonly => true ).collect do | each |
          each.datapath_id.to_s
        end
      end

      def show parameters
        raise BadRequestError.new "Datapath id must be specified." if parameters[ :id ].nil?

        datapath_id = convert_datapath_id parameters[ :id ]

        begin
          switch = DB::Switch.find( datapath_id.to_i, :readonly => true )
        rescue ActiveRecord::RecordNotFound
          raise NotFoundError.new "Not found the specified agent (datapath id #{ datapath_id } is not exists)."
        end
	{ :id => switch.datapath_id.to_s, :registered_at => switch.registered_at.to_s( :db ) }
      end
    
    end

  end

  class SwitchPort
    class << self
      def list parameters
	raise BadRequestError.new "Datapath id must be specified." if parameters[ :id ].nil?

	datapath_id = convert_datapath_id parameters[ :id ]

	DB::SwitchPort.find( :all,
			     :readonly => true,
			     :conditions => [ "datapath_id = ?", datapath_id.to_i ] ).collect do | each |
	  { :port_no => each.port_no, :name => each.name, :registered_at => each.registered_at.to_s( :db ) }
	end
      end

    end

  end

  class VirtualNetworkAgent
    class << self
      def list parameters = {}
        DB::Agent.find( :all, :readonly => true ).collect do | each |
          each.datapath_id.to_s
        end
      end

      def show parameters
        raise BadRequestError.new "Datapath id must be specified." if parameters[ :id ].nil?

        datapath_id = convert_datapath_id parameters[ :id ]

        begin
          agent = DB::Agent.find( datapath_id.to_i, :readonly => true )
        rescue ActiveRecord::RecordNotFound
          raise NotFoundError.new "Not found the specified agent (datapath id #{ datapath_id } is not exists)."
        end
	{ :id => agent.datapath_id.to_s, :uri => agent.uri }
      end
    
    end

  end

  class VxlanTunnelEndpoint
    class << self
      def list parameters = {}
        DB::TunnelEndpoint.find( :all, :readonly => true ).collect do | each |
          each.datapath_id.to_s
	end
      end

      def show parameters
        raise BadRequestError.new "Datapath id must be specified." if parameters[ :id ].nil?

        datapath_id = convert_datapath_id parameters[ :id ]

        begin
          tunnel_endpoint = DB::TunnelEndpoint.find( datapath_id.to_i, :readonly => true )
        rescue ActiveRecord::RecordNotFound
          raise NotFoundError.new "Not found the specified agent (datapath id #{ datapath_id } is not exists)."
        end
	{ :id => tunnel_endpoint.datapath_id.to_s, :local_address => tunnel_endpoint.local_address, :local_port => tunnel_endpoint.local_port }
      end

    end

  end

end
