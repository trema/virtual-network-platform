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
require 'errors'
require 'db'
require 'convert'
require 'log'

class Network

  class << self
    def create parameters
      slice_id = convert_slice_id parameters[ :id ]
      description = convert_description parameters[ :description ]

      logger.debug "#{ __FILE__ }:#{ __LINE__ }: create a new network (slice_id = #{ slice_id }, description = '#{ description }')"

      if slice_id.nil?
        slice_id = DB::Slice.generate_id
      else
        raise DuplicatedSlice.new slice_id if DB::Slice.exists?( slice_id )
      end
      if DB::OverlayNetwork.exists?( slice_id )
        begin
          DB::OverlayNetwork.delete( slice_id )
        rescue
          raise NetworkManagementError.new
        end
      end
      network = DB::OverlayNetwork.new
      network.slice_id = slice_id
      network.save!
      begin
        slice = DB::Slice.new
        slice.id = slice_id
        slice.description = description
        if update_transaction?
          logger.debug "#{ __FILE__ }:#{ __LINE__ }: update-transaction begin (slice_id = #{ slice_id })"
          slice.state = DB::SLICE_STATE_PREPARING_TO_UPDATE
        else
          slice.state = DB::SLICE_STATE_CONFIRMED
        end
        slice.save!
        { :id => slice.id, :description => slice.description, :state => slice.state.to_s }
      rescue ActiveRecord::StatementInvalid
        DB::OverlayNetwork.delete( slice_id )
        raise NetworkManagementError.new
      end
    end

    def update parameters
      raise BadRequestError.new "Slice id must be specified." if parameters[ :id ].nil?
      raise BadRequestError.new "Description must be specified." if parameters[ :description ].nil?

      slice_id = convert_slice_id parameters[ :id ]
      description = convert_description parameters[ :description ]

      logger.debug "#{ __FILE__ }:#{ __LINE__ }: update the network (slice_id = #{ slice_id }, description = '#{ description }')"

      slice = find_slice( slice_id )
      raise NetworkManagementError.new if slice.state.failed?
      raise BusyHereError.new  unless slice.state.can_update?

      DB::Slice.update_all(
        [ "description = ?", description ],
        [ "id = ? AND ( state = ? OR state = ? )",
          slice_id,
          DB::SLICE_STATE_CONFIRMED, DB::SLICE_STATE_READY_TO_UPDATE ] )
    end

    def destroy parameters
      raise BadRequestError.new "Slice id must be specified." if parameters[ :id ].nil?

      slice_id = convert_slice_id parameters[ :id ]

      logger.debug "#{ __FILE__ }:#{ __LINE__ }: destroy the network (slice_id = #{ slice_id })"

      slice = find_slice( slice_id )
      raise NetworkManagementError.new if slice.state.failed?
      raise BusyHereError unless slice.state.can_destroy?

      destroy_slice slice_id do
        destroy_all_ports slice_id do
          destroy_all_mac_addresses slice_id
        end
      end
      if not DB::Port.exists?( [ "slice_id = ? AND type = ?", slice_id, DB::PORT_TYPE_OVERLAY ] )
        begin
          DB::OverlayNetwork.delete( slice_id )
        rescue
          raise NetworkManagementError.new
        end
      end
    end

    def list parameters = {}
      datapath_id = convert_datapath_id parameters[ :datapath_id ]

      logger.debug "#{ __FILE__ }:#{ __LINE__ }: list of networks (datapath_id = #{ datapath_id })"

      if datapath_id.nil?
        id_by_dpid = nil
      else
        id_by_dpid = DB::Port.find( :all,
                                    :readonly => true,
                                    :select => 'DISTINCT slice_id',
                                    :conditions => [ "datapath_id = ? AND type = ?",
                                                     datapath_id.to_i, DB::PORT_TYPE_CUSTOMER ] ).collect do | each |
          each.slice_id
        end
      end

      responses = []
      DB::Slice.find( :all, :readonly => true ).each do | each |
        if id_by_dpid.nil? or id_by_dpid.include?( each.id )
          response = { :id => each.id, :description => each.description, :state => each.state.to_s }
          response[ :updated_at ] = each.updated_at.to_s( :db ) unless parameters[ :require_updated_at ].nil?
          responses << response
        end
      end
      responses
    end

    def show parameters
      raise BadRequestError.new "Slice id must be specified." if parameters[ :id ].nil?

      slice_id = convert_slice_id parameters[ :id ]

      logger.debug "#{ __FILE__ }:#{ __LINE__ }: show details of network (slice_id = #{ slice_id })"

      slice = find_slice( slice_id )
      response = { :id => slice.id, :description => slice.description, :state => slice.state.to_s }
      response[ :updated_at ] = slice.updated_at.to_s( :db ) unless parameters[ :require_updated_at ].nil?
      response
    end

    def reset parameters
      raise BadRequestError.new "Slice id must be specified." if parameters[ :id ].nil?

      slice_id = convert_slice_id parameters[ :id ]

      logger.debug "#{ __FILE__ }:#{ __LINE__ }: reset the network (slice_id = #{ slice_id })"

      slice = find_slice( slice_id )
      if parameters[ :force ].nil? or parameters[ :force ] == false
        raise BusyHereError unless slice.state.can_reset?
      end

      reset_slice slice_id do
        reset_all_ports slice_id do
          reset_all_mac_addresses slice_id
        end
      end
    end

    def failed parameters
      raise BadRequestError.new "Slice id must be specified." if parameters[ :id ].nil?

      slice_id = convert_slice_id parameters[ :id ]

      logger.debug "#{ __FILE__ }:#{ __LINE__ }: set to 'failed' status to the network (slice_id = #{ slice_id })"

      slice = find_slice( slice_id )
      if slice.state.updating?
        DB::Slice.update_all(
          [ "state = ?", DB::SLICE_STATE_UPDATE_FAILED ],
          [ "id = ? AND state = ?",
            slice_id,
            DB::SLICE_STATE_UPDATING ] )
      elsif slice.state.destroying?
        DB::Slice.update_all(
          [ "state = ?", DB::SLICE_STATE_DESTROY_FAILED ],
          [ "id = ? AND state = ?",
            slice_id,
            DB::SLICE_STATE_DESTROYING ] )
      else
        raise NetworkManagementError.new
      end
    end

    def update_transaction_end parameters
      raise BadRequestError.new "Slice id must be specified." if parameters[ :id ].nil?

      slice_id = convert_slice_id parameters[ :id ]

      logger.debug "#{ __FILE__ }:#{ __LINE__ }: update-transaction end (slice_id = #{ slice_id })"

      slice = find_slice( slice_id )

      DB::Port.update_all(
        [ "state = ?", DB::PORT_STATE_READY_TO_UPDATE ],
          [ "slice_id = ? AND state = ?", slice_id, DB::PORT_STATE_PREPARING_TO_DESTROY ] )
      DB::Slice.update_all(
        [ "state = ?", DB::SLICE_STATE_READY_TO_UPDATE ],
        [ "id = ? AND state = ?",
          slice_id,
          DB::SLICE_STATE_PREPARING_TO_UPDATE ] )
    end

    def create_port parameters
      # require param.
      raise BadRequestError.new "Slice id must be specified." if parameters[ :net_id ].nil?
      raise BadRequestError.new "Datapath id must be specified." if parameters[ :datapath_id ].nil?
      raise BadRequestError.new "Port number or name is required." if parameters[ :number ].nil? and parameters[ :name ].nil?
      if not parameters[ :number ].nil? and not parameters[ :name ].nil?
        raise UnprocessableEntityError.new "Port number and name are exclusive and one of them must be provided."
      end

      # validate and convert
      port_id = convert_port_id parameters[ :id ]
      slice_id = convert_slice_id parameters[ :net_id ]
      datapath_id = convert_datapath_id parameters[ :datapath_id ]
      port_no = convert_port_no parameters[ :number ]
      port_name = convert_port_name parameters[ :name ]
      vid = convert_vid parameters[ :vid ]
      description = convert_description parameters[ :description ]

      logger.debug "#{ __FILE__ }:#{ __LINE__ }: create a new port (port_id = #{ port_id }, slice_id = #{ slice_id }, datapath_id = #{ datapath_id }, port_no = #{ port_no }, port_name = '#{ port_name }', vid = #{ vid }, description = '#{ description }')"

      slice = find_slice( slice_id )
      raise NetworkManagementError.new if slice.state.failed?
      if update_transaction?
        logger.debug "#{ __FILE__ }:#{ __LINE__ }: update-transaction (slice_id = #{ slice_id })"
        raise BusyHereError.new unless slice.state == DB::SLICE_STATE_PREPARING_TO_UPDATE
      else
        raise BusyHereError.new  unless slice.state.can_update?
      end

      # exists?
      if not port_id.nil?
        raise DuplicatedPortId.new port_id if DB::Port.exists?( [ "id = ? AND slice_id = ?", port_id, slice_id ] )
      end
      if port_no != DB::PORT_NO_UNDEFINED
        raise DuplicatedPort.new port_no if DB::Port.exists?( [ "datapath_id = ? AND port_no = ? AND vid = ?",
                                                                datapath_id.to_i, port_no, vid ] )
      end
      if not port_name.empty?
        raise UnprocessableEntityError.new if /^vxlan\d+$/ =~ port_name
        raise DuplicatedPort.new port_name if DB::Port.exists?( [ "datapath_id = ? AND port_name = ? AND vid = ?",
                                                                  datapath_id.to_i, port_name, vid ] )
      end

      # debug
      begin
        agent = DB::Agent.find( datapath_id.to_i )
      rescue
        agent = nil
      end
      logger.debug "agent #{ agent.nil? ? "not " : "" }found (datapath_id = #{ datapath_id })"
      if port_no != DB::PORT_NO_UNDEFINED
        # check of port_no
        conditions = [ "datapath_id = ? AND port_no = ?", datapath_id.to_i, port_no ]
      else
        # check of port_name
        conditions = [ "datapath_id = ? AND name = ?", datapath_id.to_i, port_name ]
      end
      port = DB::SwitchPort.find( :first, :readonly => true, :conditions => conditions )
      logger.debug "switch port #{ port.nil? ? "not " : "" }found (datapath_id = #{ datapath_id }, port_no = #{ port_no }, port_name = '#{ port_name }')"

      # create port
      update_slice slice_id, update_transaction? do
        port = DB::Port.new
        port.id = port_id if not port_id.nil?
        port.slice_id = slice_id
        port.datapath_id = datapath_id
        port.port_no = port_no
        port.port_name = port_name
        port.vid = vid
        port.type = DB::PORT_TYPE_CUSTOMER
        port.description = description
        port.state = DB::PORT_STATE_READY_TO_UPDATE
        port.save!
        port_id = port.id
        add_overlay_ports slice_id
      end
      { :id => port_id }
    end

    def show_ports parameters
      raise BadRequestError.new "Slice id must be specified." if parameters[ :net_id ].nil?
      slice_id = convert_slice_id parameters[ :net_id ]
      datapath_id = convert_datapath_id parameters[ :datapath_id ]

      logger.debug "#{ __FILE__ }:#{ __LINE__ }: list of ports (slice_id = #{ slice_id }, datapath_id = #{ datapath_id })"

      find_slice( slice_id )

      conditions = [ "slice_id = ? AND type = ?", slice_id, DB::PORT_TYPE_CUSTOMER ]
      if not datapath_id.nil?
        conditions[ 0 ] << " AND datapath_id = ?"
        conditions << datapath_id.to_i
      end
      DB::Port.find( :all,
                     :readonly => true,
                     :select => 'id, datapath_id, port_no, port_name, vid, type as port_type, description, state, updated_at',
                     :conditions => conditions ).collect do | each |
        response = {
          :id => each.id,
          :datapath_id => each.datapath_id.to_s,
          :number => each.port_no,
          :name => each.port_name,
          :vid => each.vid,
          :type => each.type.to_s,
          :description => each.description,
          :state => each.state.to_s
        }
        response[ :updated_at ] = each.updated_at.to_s( :db ) unless parameters[ :require_updated_at ].nil?
        response
      end
    end

    def show_port parameters
      raise BadRequestError.new "Slice id must be specified." if parameters[ :net_id ].nil?
      raise BadRequestError.new "Port id must be specified." if parameters[ :id ].nil?
      slice_id = convert_slice_id parameters[ :net_id ]
      port_id = convert_port_id parameters[ :id ]

      logger.debug "#{ __FILE__ }:#{ __LINE__ }: show details of port (slice_id = #{ slice_id }, port_id = #{ port_id })"

      find_slice( slice_id )

      port = DB::Port.find( :first,
                            :readonly => true,
                            :select => 'id, datapath_id, port_no, port_name, vid, type as port_type, description, state, updated_at',
                            :conditions => [
                              "id = ? AND slice_id = ? AND type = ?",
                              port_id, slice_id, DB::PORT_TYPE_CUSTOMER ] )
      raise NoPortFound.new port_id if port.nil?
      response = {
        :id => port.id,
        :datapath_id => port.datapath_id.to_s,
        :number => port.port_no,
        :name => port.port_name,
        :vid => port.vid,
        :type => port.type.to_s,
        :description => port.description,
        :state => port.state.to_s,
      }
      response[ :updated_at ] = port.updated_at.to_s( :db ) unless parameters[ :require_updated_at ].nil?
      response
    end

    def delete_port parameters
      # require param.
      raise BadRequestError.new "Slice id must be specified." if parameters[ :net_id ].nil?
      raise BadRequestError.new "Port id must be specified." if parameters[ :id ].nil?

      # validate and convert
      port_id = convert_port_id parameters[ :id ]
      slice_id = convert_slice_id parameters[ :net_id ]

      logger.debug "#{ __FILE__ }:#{ __LINE__ }: delete the port (slice_id = #{ slice_id }, port_id = #{ port_id })"

      slice = find_slice( slice_id )
      raise NetworkManagementError.new if slice.state.failed?
      raise BusyHereError.new  unless slice.state.can_update?

      port = find_port( slice_id, port_id )
      raise NetworkManagementError.new if port.state.failed?
      raise BusyHereError unless port.state.can_delete?

      # delete port
      update_slice slice_id do
        destroy_port slice_id, port_id do
          destroy_mac_addresses slice_id, port_id
        end
        delete_overlay_ports slice_id
      end
    end

    def create_mac_address parameters
      # require param.
      raise BadRequestError.new "Slice id must be specified." if parameters[ :net_id ].nil?
      raise BadRequestError.new "Port id must be specified." if parameters[ :id ].nil?
      raise BadRequestError.new "Mac address must be specified." if parameters[ :address ].nil?

      # validate and convert
      slice_id = convert_slice_id parameters[ :net_id ]
      port_id = convert_port_id parameters[ :id ]
      mac = convert_mac parameters[ :address ]

      logger.debug "#{ __FILE__ }:#{ __LINE__ }: create a new mac-address (slice_id = #{ slice_id }, port_id = #{ port_id }, mac = #{ mac })"

      slice = find_slice( slice_id )
      raise NetworkManagementError.new if slice.state.failed?
      if update_transaction?
        logger.debug "#{ __FILE__ }:#{ __LINE__ }: update-transaction (slice_id = #{ slice_id })"
        raise BusyHereError.new unless slice.state == DB::SLICE_STATE_PREPARING_TO_UPDATE
      else
        raise BusyHereError.new  unless slice.state.can_update?
      end

      port = find_port( slice_id, port_id )
      raise NetworkManagementError.new if port.state.failed?
      raise BusyHereError unless port.state.can_update?
      datapath_id = port.datapath_id

      # exists?
      begin
        find_mac( slice_id, port_id, mac )
        raise DuplicatedMacAddress.new mac
      rescue NoMacAddressFound
      end

      # create mac
      update_slice slice_id, update_transaction? do
        update_port slice_id, port_id do
          mac_address = DB::MacAddress.new
          mac_address.slice_id = slice_id
          mac_address.port_id = port_id
          mac_address.mac = mac
          mac_address.type = DB::MAC_TYPE_LOCAL
          mac_address.state = DB::MAC_STATE_READY_TO_INSTALL
          mac_address.save!
        end
        add_mac_address_to_remotes slice_id, datapath_id, mac
      end
      { :address => mac.to_s }
    end

    def show_mac_addresses parameters
      raise BadRequestError.new "Slice id must be specified." if parameters[ :net_id ].nil?
      raise BadRequestError.new "Port id must be specified." if parameters[ :id ].nil?

      slice_id = convert_slice_id parameters[ :net_id ]
      port_id = convert_port_id parameters[ :id ]

      logger.debug "#{ __FILE__ }:#{ __LINE__ }: list of mac-addresses (slice_id = #{ slice_id }, port_id = #{ port_id })"

      find_slice( slice_id )
      find_port( slice_id, port_id )

      responses = []
      DB::MacAddress.find( :all,
                          :readonly => true,
                          :select => 'mac, type as port_type, state, updated_at',
                          :conditions => [ "slice_id = ? AND port_id = ?", slice_id, port_id ] ).each do | each |
        response = {}
        if parameters[ :require_state ].nil?
          next unless each.type == DB::MAC_TYPE_LOCAL
        else
          response[ :type ] = each.type.to_s
        end
        response[ :address ] = each.mac.to_s
        response[ :state ] = each.state.to_s
        response[ :updated_at ] = each.updated_at.to_s( :db ) unless parameters[ :require_updated_at ].nil?
        response
        responses << response
      end
      responses
    end

    def show_mac_address parameters
      raise BadRequestError.new "Slice id must be specified." if parameters[ :net_id ].nil?
      raise BadRequestError.new "Port id must be specified." if parameters[ :id ].nil?
      raise BadRequestError.new "Mac address must be specified." if parameters[ :address ].nil?

      slice_id = convert_slice_id parameters[ :net_id ]
      port_id = convert_port_id parameters[ :id ]
      mac = convert_mac parameters[ :address ]

      logger.debug "#{ __FILE__ }:#{ __LINE__ }: show details of mac-address (slice_id = #{ slice_id }, port_id = #{ port_id }, mac = #{ mac })"

      mac_address = find_mac( slice_id, port_id, mac )
      response = { :address => mac_address.mac.to_s, :state => mac_address.state.to_s }
      response[ :updated_at ] = mac_address.updated_at.to_s( :db ) unless parameters[ :require_updated_at ].nil?
      response
    end

    def show_remote_mac_addresses parameters
      raise BadRequestError.new "Slice id must be specified." if parameters[ :net_id ].nil?
      raise BadRequestError.new "Mac address must be specified." if parameters[ :address ].nil?

      slice_id = convert_slice_id parameters[ :net_id ]
      mac = convert_mac parameters[ :address ]

      logger.debug "#{ __FILE__ }:#{ __LINE__ }: list of remote mac-addresses (slice_id = #{ slice_id }, mac = #{ mac })"

      ports = {}
      DB::Port.find( :all,
                     :readonly => true,
                     :select => 'id, datapath_id',
                     :conditions => [ "slice_id = ? AND type = ?",
                                      slice_id, DB::PORT_TYPE_OVERLAY ] ).each do | each |
        ports[ each.id ] = each.datapath_id
      end

      DB::MacAddress.find( :all,
                           :readonly => true,
                           :select => 'port_id, mac, type as port_type, state, updated_at',
                           :conditions => [ "slice_id = ? AND mac = ? AND type = ?",
                             slice_id, mac.to_i, DB::MAC_TYPE_REMOTE ] ).collect do | each |
        datapath_id = ports[ each.port_id ]
        response = { :datapath_id => datapath_id, :address => each.mac.to_s, :state => each.state.to_s }
        response[ :updated_at ] = each.updated_at.to_s( :db ) unless parameters[ :require_updated_at ].nil?
        response
      end
    end

    def delete_mac_address parameters
      raise BadRequestError.new "Slice id must be specified." if parameters[ :net_id ].nil?
      raise BadRequestError.new "Port id must be specified." if parameters[ :id ].nil?
      raise BadRequestError.new "Mac address must be specified." if parameters[ :address ].nil?

      slice_id = convert_slice_id parameters[ :net_id ]
      port_id = convert_port_id parameters[ :id ]
      mac = convert_mac parameters[ :address ]

      logger.debug "#{ __FILE__ }:#{ __LINE__ }: delete the mac-address (slice_id = #{ slice_id }, port_id = #{ port_id }, mac = #{ mac })"

      slice = find_slice( slice_id )
      raise NetworkManagementError.new if slice.state.failed?
      raise BusyHereError.new  unless slice.state.can_update?

      port = find_port( slice_id, port_id )
      raise NetworkManagementError.new if port.state.failed?
      raise BusyHereError unless port.state.can_update?
      datapath_id = port.datapath_id

      mac_address = find_mac( slice_id, port_id, mac )
      raise NetworkManagementError.new if mac_address.state.failed?
      raise BusyHereError unless mac_address.state.can_delete?

      update_slice slice_id do
        update_port slice_id, port_id do
          destroy_mac_address slice_id, port_id, mac, DB::MAC_TYPE_LOCAL
        end
        delete_mac_address_from_remotes slice_id, datapath_id, mac
      end
    end

    private

    def update_transaction?
      config = Configure.instance
      not config[ 'update_transaction' ].nil? and config[ 'update_transaction' ]
    end

    def find_slice slice_id, parameters = { :readonly => true }
      begin
        slice = DB::Slice.find( slice_id, parameters )
        logger.debug "#{ __FILE__ }:#{ __LINE__ }: slice: slice-id=#{ slice_id } state=#{ slice.state.to_s }"
        slice
      rescue ActiveRecord::RecordNotFound
        raise NoSliceFound.new slice_id
      end
    end

    def update_slice slice_id, update_transaction = false, &a_proc
      if not update_transaction
        DB::Slice.update_all(
          [ "state = ?", DB::SLICE_STATE_PREPARING_TO_UPDATE ],
          [ "id = ? AND ( state = ? OR state = ? )",
            slice_id,
            DB::SLICE_STATE_CONFIRMED, DB::SLICE_STATE_READY_TO_UPDATE ] )
      end
      begin
        a_proc.call
      rescue
        DB::Slice.update_all(
          [ "state = ?", DB::SLICE_STATE_UPDATE_FAILED ],
          [ "id = ? AND state = ?", slice_id, DB::SLICE_STATE_PREPARING_TO_UPDATE ] )
        raise
      end
      if not update_transaction
        DB::Slice.update_all(
          [ "state = ?", DB::SLICE_STATE_READY_TO_UPDATE ],
          [ "id = ? AND state = ?", slice_id, DB::SLICE_STATE_PREPARING_TO_UPDATE ] )
      end
    end

    def destroy_slice slice_id, &a_proc
      DB::Slice.update_all(
        [ "state = ?", DB::SLICE_STATE_PREPARING_TO_DESTROY ],
        [ "id = ? AND state = ?", slice_id, DB::SLICE_STATE_CONFIRMED ] )
      begin
        a_proc.call
      rescue
        DB::Slice.update_all(
          [ "state = ?", DB::SLICE_STATE_DESTROY_FAILED ],
          [ "id = ? AND state = ?", slice_id, DB::SLICE_STATE_PREPARING_TO_DESTROY ] )
        raise
      end
      DB::Slice.update_all(
        [ "state = ?", DB::SLICE_STATE_READY_TO_DESTROY ],
        [ "id = ? AND state = ?", slice_id, DB::SLICE_STATE_PREPARING_TO_DESTROY ] )
    end

    def destroy_all_ports slice_id, &a_proc
      DB::Port.update_all(
        [ "state = ?", DB::SLICE_STATE_PREPARING_TO_DESTROY ],
        [ "slice_id = ? AND state = ?", slice_id, DB::PORT_STATE_CONFIRMED ] )
      a_proc.call
      DB::Port.update_all(
        [ "state = ?", DB::PORT_STATE_READY_TO_DESTROY ],
        [ "slice_id = ? AND state = ?", slice_id, DB::SLICE_STATE_PREPARING_TO_DESTROY ] )
    end

    def destroy_all_mac_addresses slice_id
      DB::MacAddress.update_all(
        [ "state = ?", DB::MAC_STATE_READY_TO_DELETE ],
        [ "slice_id = ? AND state = ?", slice_id, DB::MAC_STATE_INSTALLED ] )
    end

    def update_port slice_id, port_id, port_type = DB::PORT_TYPE_CUSTOMER, &a_proc
      DB::Port.update_all(
        [ "state = ?", DB::PORT_STATE_PREPARING_TO_UPDATE ],
        [ "slice_id = ? AND id = ? AND type = ? AND ( state = ? OR state = ? )",
          slice_id, port_id, port_type, DB::PORT_STATE_READY_TO_UPDATE, DB::PORT_STATE_CONFIRMED ] )
      begin
        a_proc.call
      rescue
        DB::Port.update_all(
          [ "state = ?", DB::PORT_STATE_UPDATE_FAILED ],
          [ "slice_id = ? AND id = ? AND type = ? AND state = ?",
            slice_id, port_id, port_type, DB::PORT_STATE_PREPARING_TO_UPDATE ] )
        raise
      end
      DB::Port.update_all(
        [ "state = ?", DB::PORT_STATE_READY_TO_UPDATE ],
        [ "slice_id = ? AND id = ? AND type = ? AND state = ?",
          slice_id, port_id, port_type, DB::PORT_STATE_PREPARING_TO_UPDATE ] )
    end

    def update_overlay_port slice_id, port_id, &a_proc
      update_port slice_id, port_id, DB::PORT_TYPE_OVERLAY, &a_proc
    end

    def destroy_port slice_id, port_id, port_type = DB::PORT_TYPE_CUSTOMER, &a_proc
      DB::Port.update_all(
        [ "state = ?", DB::SLICE_STATE_PREPARING_TO_DESTROY ],
        [ "slice_id = ? AND id = ? AND type = ? AND state = ?",
          slice_id, port_id, port_type, DB::PORT_STATE_CONFIRMED ] )
      begin
        a_proc.call
      rescue
        DB::Port.update_all(
          [ "state = ?", DB::PORT_STATE_UPDATE_FAILED ],
          [ "slice_id = ? AND id = ? AND type =? AND state = ?",
            slice_id, port_id, port_type, DB::SLICE_STATE_PREPARING_TO_DESTROY ] )
        raise
      end
      DB::Port.update_all(
        [ "state = ?", DB::PORT_STATE_READY_TO_DESTROY ],
        [ "slice_id = ? AND id = ? AND type =? AND state = ?",
          slice_id, port_id, port_type, DB::SLICE_STATE_PREPARING_TO_DESTROY ] )
    end

    def destroy_mac_addresses slice_id, port_id
      DB::MacAddress.update_all(
        [ "state = ?", DB::MAC_STATE_READY_TO_DELETE ],
        [ "slice_id = ? AND port_id = ? AND state = ?", slice_id, port_id, DB::MAC_STATE_INSTALLED ] )
    end

    def destroy_mac_address slice_id, port_id, mac, type
      DB::MacAddress.update_all(
        [ "state = ?", DB::MAC_STATE_READY_TO_DELETE ],
        [ "slice_id = ? AND port_id = ? AND mac = ? AND type = ? AND state = ?",
          slice_id, port_id, mac.to_i, type, DB::MAC_STATE_INSTALLED ] )
    end

    def reset_slice slice_id, &a_proc
      DB::Slice.transaction do
        DB::Slice.update_all(
          [ "state = ?", DB::SLICE_STATE_PREPARING_TO_UPDATE ],
          [ "id = ? AND ( state = ? OR state = ? )",
            slice_id,
            DB::SLICE_STATE_CONFIRMED,
            DB::SLICE_STATE_UPDATE_FAILED ] )
        DB::Slice.update_all(
          [ "state = ?", DB::SLICE_STATE_PREPARING_TO_DESTROY ],
          [ "id = ? AND state = ?",
            slice_id,
            DB::SLICE_STATE_DESTROY_FAILED ] )
        a_proc.call
        DB::Slice.update_all(
          [ "state = ?", DB::SLICE_STATE_READY_TO_UPDATE ],
          [ "id = ? AND state = ?",
            slice_id,
            DB::SLICE_STATE_PREPARING_TO_UPDATE ] )
        DB::Slice.update_all(
          [ "state = ?", DB::SLICE_STATE_READY_TO_DESTROY ],
          [ "id = ? AND state = ?",
            slice_id,
            DB::SLICE_STATE_PREPARING_TO_DESTROY ] )
      end
    end

    def reset_all_ports slice_id, &a_proc
      DB::Port.transaction do
        DB::Port.update_all(
          [ "state = ?", DB::PORT_STATE_READY_TO_UPDATE ],
          [ "slice_id = ? AND ( state = ? OR state = ? )",
            slice_id,
            DB::PORT_STATE_CONFIRMED,
            DB::PORT_STATE_UPDATE_FAILED ] )
        DB::Port.update_all(
          [ "state = ?", DB::PORT_STATE_PREPARING_TO_DESTROY ],
          [ "slice_id = ? AND state = ?", slice_id, DB::PORT_STATE_DESTROY_FAILED ] )
        a_proc.call
        DB::Port.update_all(
          [ "state = ?", DB::PORT_STATE_READY_TO_UPDATE ],
          [ "slice_id = ? AND state = ?", slice_id, DB::PORT_STATE_PREPARING_TO_DESTROY ] )
        DB::Port.update_all(
          [ "state = ?", DB::PORT_STATE_READY_TO_DESTROY ],
          [ "slice_id = ? AND state = ?", slice_id, DB::PORT_STATE_PREPARING_TO_DESTROY ] )
      end
    end

    def reset_all_mac_addresses slice_id
      DB::MacAddress.transaction do
        DB::MacAddress.update_all(
          [ "state = ?", DB::MAC_STATE_READY_TO_INSTALL ],
          [ "slice_id = ? AND ( state = ? OR state = ? )",
            slice_id,
            DB::MAC_STATE_INSTALLED,
            DB::MAC_STATE_INSTALL_FAILED ] )
        DB::MacAddress.update_all(
          [ "state = ?", DB::MAC_STATE_READY_TO_DELETE ],
          [ "slice_id = ? AND state = ?",
            slice_id,
            DB::MAC_STATE_DELETE_FAILED ] )
      end
    end

    def get_active_overlay_port slice_id, datapath_id, port_name
      port = DB::Port.find( :first,
                            :readonly => true,
                            :select => 'id',
                            :conditions => [
                              "slice_id = ? AND datapath_id = ? AND port_name = ? AND type = ? AND ( state = ? OR state = ? OR state = ? OR state = ? )",
                              slice_id, datapath_id.to_i, port_name, DB::PORT_TYPE_OVERLAY,
                              DB::PORT_STATE_CONFIRMED,
                              DB::PORT_STATE_PREPARING_TO_UPDATE,
                              DB::PORT_STATE_READY_TO_UPDATE,
                              DB::PORT_STATE_UPDATING ] )
      if port.nil?
        nil
      else
        port.id
      end
    end

    def add_overlay_ports slice_id
      logger.debug "#{ __FILE__ }:#{ __LINE__ }: Adding overlay ports (slice_id = #{ slice_id })"

      ports = get_active_ports slice_id
      switches = get_active_switches_from ports
      return if switches.size <= 1
      mac_addresses = get_active_mac_addresses slice_id

      switches.each do | datapath_id |
        remote_mac_addresses = []
        mac_addresses.each_pair do | mac_address, port_id |
          remote_datapath_id = ports[ port_id ]
          if remote_datapath_id.nil?
            logger.error "Failed to retrieve datapath_id (slice_id = #{ slice_id }, port_id = #{ port_id }, mac_address = #{ mac_address })."
            next
          end
          next if datapath_id == remote_datapath_id
          remote_mac_addresses.push mac_address
        end
        add_overlay_port( slice_id, datapath_id, remote_mac_addresses )
      end
    end

    def add_overlay_port slice_id, datapath_id, remote_mac_addresses
      logger.debug "#{ __FILE__ }:#{ __LINE__ }: Adding an overlay port (slice_id = #{ slice_id }, datapath_id = #{ datapath_id }, remote_mac_addresses = [ #{ remote_mac_addresses.join( "," ) })"

      port_name = "vxlan%u" % slice_id

      overlay_port_id = get_active_overlay_port( slice_id, datapath_id, port_name )
      if not overlay_port_id.nil?
        logger.debug "#{ __FILE__ }:#{ __LINE__ }: An overlay port already exists (slice_id = #{ slice_id }, datapath_id = #{ datapath_id })."
        return
      end

      begin
        overlay_port = DB::Port.new
        overlay_port.slice_id = slice_id
        overlay_port.datapath_id = datapath_id
        overlay_port.port_no = DB::PORT_NO_UNDEFINED
        overlay_port.port_name = port_name
        overlay_port.vid = DB::VLAN_ID_UNSPECIFIED
        overlay_port.type = DB::PORT_TYPE_OVERLAY
        overlay_port.description = "generated by Configuration Frontend"
        overlay_port.state = DB::PORT_STATE_READY_TO_UPDATE
        overlay_port.save!
      rescue
        logger.error "Failed to retrieve overlay port id (slice_id = #{ slice_id }, datapath_id = #{ datapath_id })."
        raise
      end

      overlay_port_id = get_active_overlay_port( slice_id, datapath_id, port_name )
      if overlay_port_id.nil?
        raise NetworkManagementError.new "Failed to retrieve overlay port id (slice_id = #{ slice_id }, datapath_id = #{ datapath_id })."
      end

      remote_mac_addresses.each do | each |
        begin
          mac_address = DB::MacAddress.new
          mac_address.slice_id = slice_id
          mac_address.port_id = overlay_port_id
          mac_address.mac = each
          mac_address.type = DB::MAC_TYPE_REMOTE
          mac_address.state = DB::MAC_STATE_READY_TO_INSTALL
          mac_address.save!
        rescue
          logger.error "Failed to insert remote MAC address to an overlay port (slice_id = #{ slice_id }, datapath_id = #{ datapath_id }, port_id = #{ port_id }, mac = #{ mac_address })."
           raise
        end
      end
    end

    def delete_overlay_ports slice_id
      logger.debug "#{ __FILE__ }:#{ __LINE__ }: Deleting overlay ports (slice_id = #{ slice_id })"
      get_inactive_switches( slice_id ).each do | each |
        delete_overlay_port slice_id, each
      end
      switches = get_active_switches( slice_id )
      if switches.size == 1
        delete_overlay_port slice_id, switches.first
      end
    end

    def delete_overlay_port slice_id, datapath_id
      port_name = "vxlan%u" % slice_id
      overlay_port_id = get_active_overlay_port( slice_id, datapath_id, port_name )
      if overlay_port_id.nil?
        logger.debug "#{ __FILE__ }:#{ __LINE__ }: An overlay port does not exist (slice_id = #{ slice_id }, datapath_id = #{ datapath_id })."
        return
      end
      destroy_port slice_id, overlay_port_id, DB::PORT_TYPE_OVERLAY do
        destroy_mac_addresses slice_id, overlay_port_id
      end

    end

    def get_active_switches slice_id
      switches = []
      DB::Port.find( :all,
                     :readonly => true,
                     :select => 'DISTINCT datapath_id',
                     :conditions => [
                       "slice_id = ? AND type = ? AND ( state = ? OR state = ? OR state = ? OR state = ? )",
                       slice_id, DB::PORT_TYPE_CUSTOMER,
                       DB::PORT_STATE_CONFIRMED,
                       DB::PORT_STATE_PREPARING_TO_UPDATE,
                       DB::PORT_STATE_READY_TO_UPDATE,
                       DB::PORT_STATE_UPDATING ] ).each do | each |
        switches << each.datapath_id
      end
      switches
    end

    def get_inactive_switches slice_id
      switches = []
      DB::Port.find( :all,
                     :readonly => true,
                     :select =>
                       "datapath_id, SUM(CASE WHEN state = %u OR state = %u OR state = %u OR state = %u THEN 1 ELSE 0 END) as sum" %
                       [ DB::PORT_STATE_CONFIRMED,
                         DB::PORT_STATE_PREPARING_TO_UPDATE,
                         DB::PORT_STATE_READY_TO_UPDATE,
                         DB::PORT_STATE_UPDATING ],
                     :conditions => [
                       "slice_id = ? AND type = ?",
                       slice_id, DB::PORT_TYPE_CUSTOMER ],
                     :group => "datapath_id" ).each do | each |
        switches << each.datapath_id if each.sum.to_i == 0
      end
      switches
    end

    def get_active_switches_from ports
      switches = []
      ports.each_value do | each |
        switches << each
      end
      switches.uniq
    end

    def get_active_ports slice_id
      ports = {}
      DB::Port.find( :all,
                     :readonly => true,
                     :select => 'id, datapath_id',
                     :conditions => [
                       "slice_id = ? AND type = ? AND ( state = ? OR state = ? OR state = ? OR state = ? )",
                       slice_id, DB::PORT_TYPE_CUSTOMER,
                       DB::PORT_STATE_CONFIRMED,
                       DB::PORT_STATE_PREPARING_TO_UPDATE,
                       DB::PORT_STATE_READY_TO_UPDATE,
                       DB::PORT_STATE_UPDATING ] ).each do | each |
        ports[ each.id ] = each.datapath_id
      end
      ports
    end

    def get_active_mac_addresses slice_id
      mac_addresses = {}
      DB::MacAddress.find( :all,
                           :readonly => true,
                           :select => 'port_id, mac',
                           :conditions => [
                             "slice_id = ? AND type = ? AND ( state = ? OR state = ? OR state = ? )",
                             slice_id, DB::MAC_TYPE_LOCAL,
                             DB::MAC_STATE_INSTALLED,
                             DB::MAC_STATE_READY_TO_INSTALL,
                             DB::MAC_STATE_INSTALLING ] ).each do | each |
        if mac_addresses.has_key? each.mac
          logging.debug "get_active_mac_addresses: duplicated mac address #{ each.mac }: ignored"
        else
          mac_addresses[ each.mac ] = each.port_id
        end
      end
      mac_addresses
    end

    def find_port slice_id, port_id
      port = DB::Port.find( :first,
                            :readonly => true,
                            :select => 'id, datapath_id, port_no, port_name, vid, type as port_type, description, state, updated_at',
                            :conditions => [
                              "slice_id = ? AND id = ? AND type = ?",
                              slice_id, port_id, DB::PORT_TYPE_CUSTOMER ] )
      raise NoPortFound.new port_id if port.nil?
      logger.debug "#{ __FILE__ }:#{ __LINE__ }: port: slice-id=#{ slice_id } port-id #{ port_id } state=#{ port.state.to_s }"
      port
    end

    def find_mac slice_id, port_id, mac
      mac_address = DB::MacAddress.find( :first,
                                         :readonly => true,
                                         :select => 'mac, type as port_type, state, updated_at',
                                         :conditions => [ "slice_id = ? AND port_id = ? AND mac = ? AND type = ?",
                                           slice_id, port_id, mac.to_i, DB::MAC_TYPE_LOCAL ] )

      raise NoMacAddressFound.new mac if mac_address.nil?
      logger.debug "#{ __FILE__ }:#{ __LINE__ }: port: slice-id=#{ slice_id } port-id #{ port_id } mac = #{ mac_address.mac.to_s } state=#{ mac_address.state.to_s }"
      mac_address
    end

    def add_mac_address_to_remote slice_id, datapath_id, mac
      port_name = "vxlan%u" % slice_id
      overlay_port_id = get_active_overlay_port( slice_id, datapath_id, port_name )
      if overlay_port_id.nil?
        logger.debug "#{ __FILE__ }:#{ __LINE__ }: An overlay port does not exist (slice_id = #{ slice_id }, datapath_id = #{ datapath_id })."
        return
      end
      update_overlay_port slice_id, overlay_port_id do
        mac_address = DB::MacAddress.new
        mac_address.slice_id = slice_id
        mac_address.port_id = overlay_port_id
        mac_address.mac = mac
        mac_address.type = DB::MAC_TYPE_REMOTE
        mac_address.state = DB::MAC_STATE_READY_TO_INSTALL
        mac_address.save!
      end
    end

    def add_mac_address_to_remotes slice_id, datapath_id, mac
      get_active_switches( slice_id ).each do | each |
        next if each == datapath_id
        add_mac_address_to_remote slice_id, each, mac
      end
    end

    def delete_mac_address_from_remote slice_id, datapath_id, mac
      port_name = "vxlan%u" % slice_id
      overlay_port_id = get_active_overlay_port( slice_id, datapath_id, port_name )
      if overlay_port_id.nil?
        logger.debug "#{ __FILE__ }:#{ __LINE__ }: An overlay port does not exist (slice_id = #{ slice_id }, datapath_id = #{ datapath_id })."
        return
      end
      update_overlay_port slice_id, overlay_port_id do
        destroy_mac_address slice_id, overlay_port_id, mac, DB::MAC_TYPE_REMOTE
      end
    end

    def delete_mac_address_from_remotes slice_id, datapath_id, mac
      get_active_switches( slice_id ).each do | each |
        next if each == datapath_id
        delete_mac_address_from_remote slice_id, each, mac
      end
    end

    def logger
      Log.instance
    end

  end

end
