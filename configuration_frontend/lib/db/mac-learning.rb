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

  MAC_LEARNING_DISABLE  = 0
  MAC_LEARNING_ENABLE   = 1
  MAC_LEARNING_MAX = 2

  class MacLearning
    def initialize mac_learning
      @mac_learning = mac_learning
      raise StandardError.new "Mac learning (#{ mac_learning }) is illegal range." unless ( @mac_learning >= 0 and @mac_learning < MAC_LEARNING_MAX )
    end

    def to_i
      @mac_learning
    end

    def to_s
      names = {
        MAC_LEARNING_DISABLE => "disable",
        MAC_LEARNING_ENABLE  => "enable",
      }
      names[ @mac_learning ] or "MAC_LEARNING_%d" % @mac_learning
    end

    def ==( other )
      other.to_i == @mac_learning
    end

    def mac_learning?
      @mac_learning == MAC_LEARNING_ENABLE
    end

  end

end
