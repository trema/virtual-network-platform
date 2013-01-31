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

module DB

  PORT_TYPE_CUSTOMER = 0
  PORT_TYPE_OVERLAY  = 1
  PORT_TYPE_MAX      = 2
  PORT_TYPE_ANY      = 255

  class PortType
    def initialize type
      @type = type
      raise StandardError.new "Port type (#{ type }) is illegal range." unless ( @type >= 0 and @type < PORT_TYPE_MAX )
    end

    def to_i
      @type
    end

    def to_s
      names = {
        PORT_TYPE_CUSTOMER => "customer",
        PORT_TYPE_OVERLAY  => "overlay"
      }
      names[ @type ] or "PORT_TYPE_%d" % @type
    end

    def ==( other )
      other.to_i == @type
    end

  end

end
