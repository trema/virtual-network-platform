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

require 'db/base'
require 'db/datapath-id'
require 'db/mac-learning'
require 'db/port-state'
require 'db/port-type'

module DB

  PORT_NO_UNDEFINED   = 65535
  VLAN_ID_UNSPECIFIED = 65535

  class Port < Base
    def type
      PortType.new read_attribute( :port_type ).to_i
    end

    def type= ( value )
      write_attribute( :type, value.to_i )
    end

    def state
      PortState.new read_attribute( :state )
    end

    def state= ( value )
      write_attribute( :state, value.to_i )
    end

    def datapath_id
      DatapathId.new read_attribute( :datapath_id )
    end

    def datapath_id= ( value )
      write_attribute( :datapath_id, value.to_i )
    end

    def mac_learning
      MacLearning.new read_attribute( :mac_learning )
    end

    def mac_learning= ( value )
      if value.nil?
        value = MAC_LEARNING_DISABLE
      end
      write_attribute( :mac_learning, value.to_i )
    end

  end

end
