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

  class Mac
    def initialize mac
      case mac
        when Integer
          @mac = mac
        when /^(?:(?:[[:xdigit:]]{1,2}):){5}[[:xdigit:]]{1,2}/
          @mac = 0
          mac.split(":").each do | each |
            i = each.hex
            @mac = ( @mac << 8 ) + i
          end
        else
          raise StandardError.new "Invalid MAC address (#{ mac })"
      end
      raise StandardError.new "Invalid MAC address (#{ mac })" unless ( @mac >= 0 and @mac <= 0xffffffffffff )
    end

    def to_i
      @mac
    end

    def to_s
      mac_i = @mac
      a = []
      while a.length < 6
        a.unshift( "%02x" % ( mac_i & 0xff ) )
        mac_i = mac_i >> 8
      end
      a.join( ":" )
    end

    def ==( other )
      other.to_i == @mac
    end

  end

end
