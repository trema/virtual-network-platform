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

  PORT_STATE_CONFIRMED            = 0
  PORT_STATE_PREPARING_TO_UPDATE  = 1
  PORT_STATE_READY_TO_UPDATE      = 2
  PORT_STATE_UPDATING             = 3
  PORT_STATE_UPDATE_FAILED        = 4
  PORT_STATE_PREPARING_TO_DESTROY = 5
  PORT_STATE_READY_TO_DESTROY     = 6
  PORT_STATE_DESTROYING           = 7
  PORT_STATE_DESTROY_FAILED       = 8
  PORT_STATE_DESTROYED            = 9
  PORT_STATE_MAX                  = 10

  class PortState
    def initialize state
      @state = state
      raise StandardError.new "Port state (#{ state }) is illegal range." unless ( @state >= 0 and @state < PORT_STATE_MAX )
    end

    def to_i
      @state
    end

    def to_s
      names = {
        PORT_STATE_CONFIRMED            => "confirmed",
        PORT_STATE_PREPARING_TO_UPDATE  => "preparing-to-update",
        PORT_STATE_READY_TO_UPDATE      => "ready-to-update",
        PORT_STATE_UPDATING             => "updating",
        PORT_STATE_UPDATE_FAILED        => "update-failed",
        PORT_STATE_PREPARING_TO_DESTROY => "preparing-to-destroy",
        PORT_STATE_READY_TO_DESTROY     => "ready-to-destroy",
        PORT_STATE_DESTROYING           => "destroying",
        PORT_STATE_DESTROY_FAILED       => "destroy-failed",
        PORT_STATE_DESTROYED            => "destroyed"
      }
      names[ @state ] or "PORT_STATE_%d" % @state
    end

    def ==( other )
      @state == other.to_i
    end

    def failed?
      @state == PORT_STATE_UPDATE_FAILED or
      @state == PORT_STATE_DESTROY_FAILED
    end

    def can_update?
      @state == PORT_STATE_CONFIRMED or
      @state == PORT_STATE_READY_TO_UPDATE
    end

    def can_delete?
      @state == PORT_STATE_CONFIRMED
    end

    def can_reset?
      @state != PORT_STATE_PREPARING_TO_UPDATE and
      @state != PORT_STATE_PREPARING_TO_DESTROY
    end

  end

end
