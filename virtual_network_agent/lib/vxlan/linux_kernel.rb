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

require 'systemu'
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

        def mtu vni, mtu
          IpLink.mtu vni, mtu
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

        def mtu vni, mtu
          port_name = name vni
          ip_link 'set', [ port_name, 'mtu', mtu ]
        end

        private

        def config
          Vxlan::Configure.instance[ 'linux_kernel' ]
        end

        def ip_link command, options = []
          status, result, error = systemu "#{ config[ 'ip' ] } link #{ command } #{ options.join ' ' }"
          raise "#{ result } #{ error } #{ status.inspect } #{ config[ 'ip' ] }" unless status.success?
          result
        end

      end

    end

  end

end
