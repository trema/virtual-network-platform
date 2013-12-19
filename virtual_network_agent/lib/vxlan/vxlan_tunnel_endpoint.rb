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
require 'vxlan/log'

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

        def list vni = nil
          VxlanCtl.list_instances vni
        end

        def exists? vni
          list( vni ).has_key? vni
        end

        def mtu vni, mtu
          device = name vni
          IpLink.mtu device, mtu
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

        def list_instances vni = nil
          list = {}
          options = [ '--quiet' ]
          if not vni.nil?
            options = [ '--vni', vni ]
          end
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
          command_options = "#{ full_path } #{ command } #{ options.join ' ' }"
          logger.debug "vxlanctl: '#{ command_options }'"
          result = ""
          Open3.popen3( "#{ full_path } #{ command } #{ options.join ' ' }" ) do | stdin, stdout, stderr |
            stdin.close
            t = Thread.start do
              result << stdout.read
            end
            error = stderr.read
            t.join
            raise "Permission denied #{ full_path }" if /Permission denied/ =~ result
            raise "#{ result } #{ full_path }" if /Failed to/ =~ result
            raise "#{ error } #{ full_path }" unless error.length == 0
          end
          result
        end

        def logger
          Log.instance
        end

      end

    end

    class IpLink
      class << self
        def mtu device, mtu
          ip_link 'set', [ device, 'mtu', mtu ]
        end

        private

        def config
          Vxlan::Configure.instance[ 'vxlan_tunnel_endpoint' ]
        end

        def ip_link command, options = []
          result = ""
          Open3.popen3( "#{ config[ 'ip' ] } link #{ command } #{ options.join ' ' }" ) do | stdin, stdout, stderr |
            stdin.close
            error = stderr.read
            result << stdout.read
            raise "#{ error } #{ config[ 'ip' ] }" unless error.length == 0
          end
          result
        end

      end

    end

  end

end
