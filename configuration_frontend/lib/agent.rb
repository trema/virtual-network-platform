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

class Agent
  class << self
    def register parameters
      raise BadReuestError.new "Datapath id must be specified." if parameters[ :datapath_id ].nil?
      raise BadReuestError.new "Agent uri must be specified." if parameters[ :control_uri ].nil?
      raise BadReuestError.new "Tunnel end point must be specified." if parameters[ :tunnel_endpoint ].nil?

      datapath_id = convert_datapath_id parameters[ :datapath_id ]
      uri = convert_uri parameters[ :control_uri ]
      tep = convert_tep parameters[ :tunnel_endpoint ]

      logger.debug "#{ __FILE__ }:#{ __LINE__ }: register a new agent (datapath_id = #{ datapath_id }, uri = #{ uri }, tep = #{ tep })"

      DB::Agent.transaction do
        begin
          agent = DB::Agent.find( datapath_id.to_i )
          agent.uri = uri
          agent.save!
        rescue ActiveRecord::RecordNotFound
          agent = DB::Agent.new
          agent.datapath_id = datapath_id
          agent.uri = uri
          agent.save!
        end

        begin
          tunnel_endpoint = DB::TunnelEndpoint.find( datapath_id.to_i )
          tunnel_endpoint.tep = tep
          tunnel_endpoint.save!
        rescue ActiveRecord::RecordNotFound
          tunnel_endpoint = DB::TunnelEndpoint.new
          tunnel_endpoint.datapath_id = datapath_id
          tunnel_endpoint.tep = tep
          tunnel_endpoint.save!
        end
      end
    end

    def action parameters
      raise BadReuestError.new "Datapath id must be specified." if parameters[ :datapath_id ].nil?
      raise BadReuestError.new "Action must be specified." if parameters[ :action ].nil?

      datapath_id = convert_datapath_id parameters[ :datapath_id ]
      raise BadReuestError.new "action (#{ parameters[ :action ] }) is illegal request." unless /^reset$/i =~ parameters[ :action ]

      logger.debug "#{ __FILE__ }:#{ __LINE__ }: action='reset' (datapath_id = #{ datapath_id })"

      # reset
      DB::Slice.transaction do
        slices = DB::Port.find( :all,
                                :readonly => true,
                                :select => 'DISTINCT slice_id',
                                :conditions => [ "datapath_id = ?", datapath_id.to_i ] ).collect do | each |
          each.slice_id
        end
        reset_slices slices do
          slices.each do | slice_id |
            DB::Port.update_all(
              [ "state = ?", DB::PORT_STATE_READY_TO_UPDATE ],
              [ "slice_id = ? AND ( state = ? OR state = ? )", slice_id, DB::PORT_STATE_CONFIRMED, DB::PORT_STATE_UPDATE_FAILED ] )
            DB::Port.update_all(
              [ "state = ?", DB::PORT_STATE_READY_TO_DESTROY ],
              [ "slice_id = ? AND state = ?", slice_id, DB::PORT_STATE_DESTROY_FAILED ] )
            DB::MacAddress.update_all(
              [ "state = ?", DB::MAC_STATE_READY_TO_INSTALL ],
              [ "slice_id = ? AND ( state = ? OR state = ? )", slice_id, DB::MAC_STATE_INSTALLED, DB::MAC_STATE_INSTALL_FAILED ] )
            DB::MacAddress.update_all(
              [ "state = ?", DB::MAC_STATE_READY_TO_DELETE ],
              [ "slice_id = ? AND state = ?", slice_id, DB::MAC_STATE_DELETE_FAILED ] )
          end
        end
      end
    end

    def unregister parameters
      raise BadReuestError.new "Datapath id must be specified." if parameters[ :datapath_id ].nil?

      datapath_id = convert_datapath_id parameters[ :datapath_id ]

      logger.debug "#{ __FILE__ }:#{ __LINE__ }: unregister the agent (datapath_id = #{ datapath_id })"

      # exists?
      DB::Agent.find( datapath_id.to_i, :readonly => true )
      DB::TunnelEndpoint.find( datapath_id.to_i, :readonly => true )

      DB::Agent.transaction do
        DB::Agent.delete( datapath_id.to_i )
        DB::TunnelEndpoint.delete( datapath_id.to_i )
      end
    end

    def list parameters = {}

      logger.debug "#{ __FILE__ }:#{ __LINE__ }: list of agents"

      tunnel_endpoints = {}
      DB::TunnelEndpoint.find( :all, :readonly => true ).each do | each |
        tunnel_endpoints[ each.datapath_id.to_s ] = each.tep.to_s
      end
      DB::Agent.find( :all, :readonly => true ).collect do | each |
        { :datapath_id => each.datapath_id.to_s, :control_uri => each.uri.to_s, :tunnel_endpoint => tunnel_endpoints[ each.datapath_id.to_s ] }
      end
    end

    def show parameters
      raise BadReuestError.new "Datapath id must be specified." if parameters[ :datapath_id ].nil?

      datapath_id = convert_datapath_id parameters[ :datapath_id ]

      logger.debug "#{ __FILE__ }:#{ __LINE__ }: show details of switch (datapath_id = #{ datapath_id })"

      begin
        agent = DB::Agent.find( datapath_id.to_i, :readonly => true )
      rescue ActiveRecord::RecordNotFound
        raise NotFoundError.new "Not found the specified agent (datapath id #{ datapath_id } is not exists)."
      end

      begin
        tunnel_endpoint = DB::TunnelEndpoint.find( datapath_id.to_i, :readonly => true )
      rescue ActiveRecord::RecordNotFound
        raise NotFoundError.new "Not found the specified tunnel-endpoint (datapath id #{ datapath_id } is not exists)."
      end

      { :datapath_id => datapath_id.to_s, :control_uri => agent.uri.to_s, :tunnel_endpoint => tunnel_endpoint.tep.to_s }
    end

    private

    def reset_slices slices, &a_proc
      slices.each do | slice_id |
        slice = DB::Slice.find( slice_id, :readonly => true )
        logger.debug "#{ __FILE__ }:#{ __LINE__ }: reset slice: slice_id=#{ slice_id } state=#{ slice.state.to_s }"
        raise BusyHereError unless slice.state.can_reset?
        DB::Slice.update_all(
          [ "state = ?", DB::SLICE_STATE_PREPARING_TO_UPDATE ],
          [ "id = ? AND ( state = ? OR state = ? )", slice_id, DB::SLICE_STATE_CONFIRMED, DB::SLICE_STATE_UPDATE_FAILED ] )
        DB::Slice.update_all(
          [ "state = ?", DB::SLICE_STATE_PREPARING_TO_DESTROY ],
          [ "id = ? AND state = ?", slice_id, DB::SLICE_STATE_DESTROY_FAILED ] )
      end
      a_proc.call
      slices.each do | slice_id |
        DB::Slice.update_all(
          [ "state = ?", DB::SLICE_STATE_READY_TO_UPDATE ],
          [ "id = ? AND state = ?", slice_id, DB::SLICE_STATE_PREPARING_TO_UPDATE ] )
        DB::Slice.update_all(
          [ "state = ?", DB::SLICE_STATE_READY_TO_DESTROY ],
          [ "id = ? AND state = ?", slice_id, DB::SLICE_STATE_PREPARING_TO_DESTROY ] )
      end
    end

    def logger
      Log.instance
    end

  end

end
