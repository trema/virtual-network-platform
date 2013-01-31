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

module DB

  class Agent < Base
    set_primary_key "datapath_id"

    def datapath_id
      DatapathId.new read_attribute( :datapath_id )
    end

    def datapath_id= ( value )
      write_attribute( :datapath_id, value.to_i )
    end

    def uri
      Uri.new read_attribute( :uri )
    end

    def uri= ( value )
      write_attribute( :uri, value.to_s )
    end

  end

end
