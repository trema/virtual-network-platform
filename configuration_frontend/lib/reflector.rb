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
require 'log'

class Reflector
  class << self
    def list parameters = {}

      logger.debug "#{ __FILE__ }:#{ __LINE__ }: list of reflectors"

      tunnel_endpoints = Hash.new do | hash, key |
        hash[ key ] = []
      end
      ActiveRecord::Base.connection.select_all(
        "SELECT DISTINCT P.slice_id, T.local_address, T.local_port" +
        " FROM ports P, tunnel_endpoints T" +
        " WHERE P.datapath_id = T.datapath_id" ).each do | each |
        tunnel_endpoint = { :ip => each[ 'local_address' ], :port => each[ 'local_port' ] }
        tunnel_endpoints[ each[ 'slice_id' ].to_i ].push tunnel_endpoint
      end
      tunnel_endpoints
    end

    private

    def logger
      Log.instance
    end

  end

end
