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

require 'db/ip'

module DB

  class Tep
    def initialize tep
      raise StandardError.new "Invalid tunnel end point (#{ tep })" unless /^((?:(?:[[:digit:]]{1,3})\.){3}[[:digit:]]{1,3})(:([[:digit:]]+))?$/ =~ tep
      @ip_address = IP.new( $1 )
      if $2.nil?
        @port = nil
      else
        @port = $2.to_i
        raise "Invalid port (#{ tep })" unless ( @port >= 0 and @port <= 0xffff )
      end
    end

    def self.create ip_address, port = nil
      if port.nil?
        new ip_address.to_s
      else
        new ip_address.to_s + ':' + port.to_s
      end
    end

    def to_s
      if @port.nil?
        @ip_address.to_s
      else
        @ip_address.to_s + ':' + @port.to_s
      end
    end

    def ip_address
      @ip_address
    end

    def port
      @port
    end

    def ==( other )
      other.ip_address == @ip_address and other.port == @port
    end

  end

end
