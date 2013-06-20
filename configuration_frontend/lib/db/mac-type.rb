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

  MAC_TYPE_LOCAL  = 0
  MAC_TYPE_REMOTE = 1
  MAC_TYPE_MAX    = 2
  MAC_TYPE_ANY    = 255

  class MacType
    def initialize type
      @type = type
      raise StandardError.new "Mac address type (#{ type }) is illegal range." unless ( @type >= 0 and @type < MAC_TYPE_MAX )
    end

    def to_i
      @type
    end

    def to_s
      names = {
        MAC_TYPE_LOCAL  => "local",
        MAC_TYPE_REMOTE => "remote"
      }
      names[ @type ] or "MAC_TYPE_%d" % @type
    end

    def ==( other )
      @type == other.to_i
    end

  end

end
