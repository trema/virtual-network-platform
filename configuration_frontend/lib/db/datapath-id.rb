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

  class DatapathId
    def initialize dpid
      case dpid
        when Integer
          if dpid < 0
            @dpid = dpid + ( 1 << 64 )
          else
            @dpid = dpid
          end
        when /^0[0-7]*$/
          @dpid = dpid.oct
        when /^[1-9][[:digit:]]*$/
          @dpid = dpid.to_i
        when /^0[Xx][[:xdigit:]]+$/
          @dpid = dpid.hex
        else
          raise StandardError.new "Datapath id (#{ dpid }) is illegal format."
      end
      raise StandardError.new "Datapath id (#{ dpid }) is illegal range." unless ( @dpid >= 0 and @dpid <= 0xffffffffffffffff )
    end

    def to_i
      @dpid
    end

    def to_s
      "0x%016x" % @dpid
    end

    def ==( other )
      @dpid == other.to_i
    end

    def eql? other
      @dpid.eql? other.to_i
    end

    def hash
      @dpid.hash
    end

  end

end
