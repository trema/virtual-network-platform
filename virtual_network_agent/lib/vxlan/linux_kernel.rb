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

require 'open3'
require 'vxlan/configure'

module Vxlan

  module LinuxKernel

    class Instance
      class << self
	def add vni, address
	  IpLink.add vni, address
	  IpLink.up vni
	end

	def delete vni
	  IpLink.down vni
	  IpLink.delete vni
	end

	def name vni
	  IpLink.name vni
	end

	def list
	  IpLink.show
	end

	def exists? vni
	  list.has_key? vni
	end

      end

    end

    class IpLink
      class << self
	def name vni
	  "vxlan%u" % vni.to_i
	end

        def add vni, group
	  port_name = name vni
	  device = config[ 'device' ]
	  ip_link 'add', [ port_name, 'type', 'vxlan', 'id', vni, 'group', group, 'dev', device ]
	end

	def delete vni
	  port_name = name vni
	  ip_link 'delete', [ port_name ]
	end

	def show up_only = true
	  list = {}
	  options = []
	  if up_only
	    options = options + [ 'up' ]
	  end
	  ip_link( 'show', options ).split( "\n").each do | row |
	    index, port_name, info = row.split( /:\s*/, 3 )
	    next if /^vxlan(\d+)$/ !~ port_name
	    vni = $1
	    state = "UNKNOWN"
	    if /state\s+(\S+)/ =~ info
	      state = $1
	    end
	    list [ vni.to_i ] = { :port_name => port_name, :state => state }
	  end
	  list
	end

	def up vni
	  port_name = name vni
	  ip_link 'set', [ port_name, 'up' ]
	end

	def down vni
	  port_name = name vni
	  ip_link 'set', [ port_name, 'down' ]
	end

	private

        def config
	  Vxlan::Configure.instance[ 'linux_kernel' ]
        end

	def ip_link command, options = []
	  result = ""
	  Open3.popen3( "#{ config[ 'ip' ] } link #{ command } #{ options.join ' ' }" ) do | stdin, stdout, stderr |
	    stdin.close
	    error = stderr.read
	    result << stdout.read
	    raise "#{ error } #{ config[ 'ip' ] }" unless error.length == 0
	  end
	  unless $?.success?
	    raise "Cannot execute #{ config[ 'ip' ] } (exit status #{ $?.exitstatus })"
	  end
	  result
	end

      end

    end

  end

end
