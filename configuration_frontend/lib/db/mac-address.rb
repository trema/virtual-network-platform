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
require 'db/mac-state'
require 'db/mac-type'
require 'db/mac'

module DB

  class MacAddress < Base
    set_primary_key "mac"

    def type
      MacType.new read_attribute( :mac_type ).to_i
    end

    def type= ( value )
      write_attribute( :type, value.to_i )
    end

    def state
      MacState.new read_attribute( :state )
    end

    def state= ( value )
      write_attribute( :state, value.to_i )
    end

    def mac
      Mac.new read_attribute( :mac )
    end

    def mac= ( value )
      write_attribute( :mac, value.to_i )
    end

  end

end
