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

  MAC_STATE_INSTALLED        = 0
  MAC_STATE_READY_TO_INSTALL = 1
  MAC_STATE_INSTALLING       = 2
  MAC_STATE_INSTALL_FAILED   = 3
  MAC_STATE_READY_TO_DELETE  = 4
  MAC_STATE_DELETING         = 5
  MAC_STATE_DELETE_FAILED    = 6
  MAC_STATE_DELETED          = 7
  MAC_STATE_MAX              = 8

  class MacState
    def initialize state
      @state = state
      raise StandardError.new "Mac address state (#{ state }) is illegal range." unless ( @state >= 0 and @state < MAC_STATE_MAX )
    end

    def to_i
      @state
    end

    def to_s
      names = {
        MAC_STATE_INSTALLED        => "installed",
        MAC_STATE_READY_TO_INSTALL => "ready-to-install",
	MAC_STATE_INSTALLING       => "installing",
	MAC_STATE_INSTALL_FAILED   => "install-failed",
	MAC_STATE_READY_TO_DELETE  => "ready-to-delete",
	MAC_STATE_DELETING         => "deleting",
	MAC_STATE_DELETE_FAILED    => "delete-failed",
	MAC_STATE_DELETED          => "deleted"
      }
      names[ @state ] or "MAC_STATE_%d" % @state
    end

    def ==( other )
      other.to_i == @state
    end

    def failed?
      @state == MAC_STATE_INSTALL_FAILED or
      @state == MAC_STATE_DELETE_FAILED
    end

    def can_update?
      @state == MAC_STATE_INSTALLED or
      @state == MAC_STATE_READY_TO_INSTALL
    end

    def can_delete?
      @state == MAC_STATE_INSTALLED
    end

    def can_reset?
      @state != MAC_STATE_INSTALLING and
      @state != MAC_STATE_DELETING
    end

  end

end
