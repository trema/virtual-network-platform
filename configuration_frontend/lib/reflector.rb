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

require 'errors'
require 'db'

class Reflector
  class << self
    def initialize
      @logger = Log.instance
    end

    def list parameters = {}
      tunnel_endpoints = Hash.new do | hash, key |
        hash[ key ] = []
      end
      ActiveRecord::Base.connection.select_all(
        "SELECT DISTINCT P.slice_id, T.local_address, T.local_port" +
	" FROM ports P, tunnel_endpoints T" +
        " WHERE P.datapath_id = T.datapath_id" ).each do | each |
	tunnel_endpoint = { :ip => each.local_address, :port => each.local_port, }
	tunnel_endpoints[ each.slice_id ].push tunnel_endpoint
      end
      tunnel_endpoints
    end

    private

    def logger
      @logger
    end

  end

end
