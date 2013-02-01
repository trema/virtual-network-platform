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

begin
require 'rubygems'
rescue LoadError
end
require 'rest_client'
require 'json'

require 'configure'
require 'overlay_network'
require 'reflector'

class Notify
  class << self
    def startup
      mode_type.startup
    end

    def shutdown
      mode_type.shutdown
    end

    def config
      Configure.instance
    end

    def mode_type
      if config[ 'reflector_mode' ]
        Reflector
      else
        OverlayNetwork
      end
    end

  end

end
