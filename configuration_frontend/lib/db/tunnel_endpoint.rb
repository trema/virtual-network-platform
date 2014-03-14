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
require 'db/tep'

module DB

  class TunnelEndpoint < Base
    self.primary_key = "datapath_id"

    def datapath_id
      DatapathId.new read_attribute( :datapath_id )
    end

    def datapath_id= ( value )
      write_attribute( :datapath_id, value.to_i )
    end

    def tep
      ip_address = IP.new read_attribute( :local_address )
      port = read_attribute( :local_port )
      Tep.create ip_address, port
    end

    def tep= ( value )
      write_attribute( :local_address, value.ip_address.to_s )
      write_attribute( :local_port, value.port.to_i )
    end

  end

end
