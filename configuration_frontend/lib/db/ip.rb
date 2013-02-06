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

  class IP
    def initialize ip
      case ip
        when Integer
          @ip = ip
        when /^(?:(?:[[:digit:]]{1,3})\.){3}[[:digit:]]{1,3}$/
          @ip = 0
          ip.split(".").each do | each |
            i = each.to_i
            raise "Invalid IP address (#{ ip })" unless i < 256
            @ip = ( @ip << 8 ) + i
          end
        else
          raise "Invalid IP address (#{ ip })"
      end
      raise "Invalid IP address (#{ ip })" unless ( @ip >= 0 and @ip <= 0xffffffff )
    end

    def to_i
      @ip
    end

    def to_s
      ip_i = @ip
      a = []
      while a.length < 4
        a.unshift( "%u" % ( ip_i & 0xff ) )
        ip_i = ip_i >> 8
      end
      a.join( "." )
    end

  end

end
