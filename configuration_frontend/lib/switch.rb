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

class Switch
  class << self
    def list parameters = {}
      switch_ports = Hash.new do | hash, key |
        hash[ key ] = []
      end
      ActiveRecord::Base.connection.select_all(
        "SELECT datapath_id, port_no, name" +
	" FROM switch_ports" ).each do | each |
	switch_port = { :port_no => each[ 'port_no' ].to_i, :name => each[ 'name' ] }
	datapath_id = "0x%016x" % each[ 'datapath_id' ].to_i
	switch_ports[ datapath_id ].push switch_port
      end
      switch_ports
    end

    private

    def logger
      Log.instance
    end

  end

end
