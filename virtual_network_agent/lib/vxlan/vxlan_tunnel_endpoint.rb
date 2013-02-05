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

  module VxlanTunnelEndpoint

    class Instance
      class << self
	def add vni, address
	  VxlanCtl.add_instance vni, address
	end

	def delete vni
	  VxlanCtl.delete_instance vni
	end

	def name vni
	  VxlanCtl.name vni
	end

	def list
	  VxlanCtl.list_instances
	end

	def exists? vni
	  list.has_key? vni
	end

      end

    end

    class VxlanCtl
      class << self
	def name vni
	  "vxlan%u" % vni.to_i
	end

	def add_instance vni, address
	  options = [ '--vni', vni ]
	  if not address.nil?
	    options = options + [ '--ip', address ]
	  end
	  vxlanctl '--add_instance', options
	end

	def delete_instance vni
	  options = [ '--vni', vni ]
	  vxlanctl '--del_instance', options
	end

	def list_instances
	  list = {}
	  options = [ '--quiet' ]
	  vxlanctl( '--list_instances', options ).split( "\n").each do | row |
	    row = $1 if /^\s*(\S+(?:\s+\S+)*)\s*$/ =~ row
	    vni, address, port, aging_time, state = row.split( /\s*\|\s*/, 5 )
	    port = 0 if port == '-'
	    list [ vni.hex ] = { :address => address, :port => port.to_i, :aging_time => aging_time.to_i, :state => state }
	  end
	  list
	end

	private

	def config
	  Vxlan::Configure.instance[ 'vxlan_tunnel_endpoint' ]
	end

	def vxlanctl command, options = []
	  full_path = config[ 'vxlanctl' ]
          if %r,^/, !~ full_path
            full_path = File.dirname( __FILE__ ) + '/../../' + full_path
          end
	  result = ""
	  Open3.popen3( "#{ full_path } #{ command } #{ options.join ' ' }" ) do | stdin, stdout, stderr |
	    stdin.close
	    error = stderr.read
	    result << stdout.read
	    raise "Permission denied #{ full_path }" if /Permission denied/ =~ result
	    raise "#{ result } #{ full_path }" if /Failed to/ =~ result
	    raise "#{ error } #{ full_path }" unless error.length == 0
	  end
	  result
	end

      end

    end

  end

end
