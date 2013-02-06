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

  SLICE_STATE_CONFIRMED            = 0
  SLICE_STATE_PREPARING_TO_UPDATE  = 1
  SLICE_STATE_READY_TO_UPDATE      = 2
  SLICE_STATE_UPDATING             = 3
  SLICE_STATE_UPDATE_FAILED        = 4
  SLICE_STATE_PREPARING_TO_DESTROY = 5
  SLICE_STATE_READY_TO_DESTROY     = 6
  SLICE_STATE_DESTROYING           = 7
  SLICE_STATE_DESTROY_FAILED       = 8
  SLICE_STATE_DESTROYED            = 9
  SLICE_STATE_MAX                  = 10

  class SliceState
    def initialize state
      @state = state
      raise StandardError.new "Slice state (#{ state }) is illegal range." unless ( @state >= 0 and @state < SLICE_STATE_MAX )
    end

    def to_i
      @state
    end

    def to_s
      names = {
        SLICE_STATE_CONFIRMED            => "confirmed",
        SLICE_STATE_PREPARING_TO_UPDATE  => "preparing-to-update",
        SLICE_STATE_READY_TO_UPDATE      => "ready-to-update",
        SLICE_STATE_UPDATING             => "updating",
        SLICE_STATE_UPDATE_FAILED        => "update-failed",
        SLICE_STATE_PREPARING_TO_DESTROY => "preparing-to-destroy",
        SLICE_STATE_READY_TO_DESTROY     => "ready-to-destroy",
        SLICE_STATE_DESTROYING           => "destroying",
        SLICE_STATE_DESTROY_FAILED       => "destroy-failed",
        SLICE_STATE_DESTROYED            => "destroyed"
      }
      names[ @state ] or "SLICE_STATE_%d" % @state
    end

    def ==( other )
      other.to_i == @state
    end

    def failed?
      @state == SLICE_STATE_UPDATE_FAILED or
      @state == SLICE_STATE_DESTROY_FAILED
    end

    def can_update?
      @state == SLICE_STATE_CONFIRMED or
      @state == SLICE_STATE_READY_TO_UPDATE
    end

    def can_destroy?
      @state == SLICE_STATE_CONFIRMED
    end

    def can_reset?
      @state != SLICE_STATE_PREPARING_TO_UPDATE and
      @state != SLICE_STATE_PREPARING_TO_DESTROY
    end

  end

end
