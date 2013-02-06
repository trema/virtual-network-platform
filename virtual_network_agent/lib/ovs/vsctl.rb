# # Copyright (C) 2013 NEC Corporation
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
require 'ovs/configure'

module OVS

  class Bridge
    class << self
      def list
        Vsctl.list_br
      end

      def datapath_id
        Vsctl.datapath_id
      end

      def exists? br
        list.each do | each |
          return true if br == each
        end
        false
      end

    end

  end

  class Port
    class << self
      def add port_name
        Vsctl.add_port port_name
      end

      def delete port_name
        Vsctl.delete_port port_name
      end

      def list
        Vsctl.list_ports
      end

      def exists? port_name
        list.each do | each |
          return true if port_name == each
        end
        false
      end

    end

  end

  class Interface
    class << self
      def list
        Vsctl.list_ifaces
      end

      def statistics iface
        Vsctl.iface_statistics iface
      end

    end
  end

  class Vsctl
    class << self
      def list_br
        ovs_vsctl( [ '--timeout=1' ], 'list-br' ).split( "\n")
      end

      def add_port port_name
        ovs_vsctl [ '--timeout=1', '--', '--may-exist' ], 'add-port', [ bridge, port_name ]
      end

      def delete_port port_name
        ovs_vsctl [ '--timeout=1', '--', '--if-exists' ], 'del-port', [ bridge, port_name ]
      end

      def list_ports
        ovs_vsctl( [ '--timeout=1' ], 'list-ports', [ bridge ] ).split( "\n")
      end

      def list_ifaces
        ovs_vsctl( [ '--timeout=1' ], 'list-ifaces', [ bridge ] ).split( "\n")
      end

      def datapath_id
        s = ovs_vsctl( [ '--timeout=1' ], 'get', [ 'bridge', bridge, 'other-config:datapath-id' ] ).split( "\n").first
        if /^"([[:xdigit:]]+)"$/ =~ s
          $1.hex
        else
          raise "Can not get datapath id"
        end
      end

      def iface_statistics iface
        s = ovs_vsctl( [ '--timeout=1' ], 'get', [ 'interface', iface, 'statistics' ] )
        if /^\{(.*)\}$/ =~ s
          h = {}
          $1.split( /\s*,\s*/ ).each do | each |
            key, value = each.split( "=", 2 )
            h[ key ] = value.to_i
          end
          h
        else
          raise "Can not get interface statistics"
        end
      end

      private

      def config
        Configure.instance
      end

      def bridge
        config[ 'bridge' ]
      end

      def ovs_vsctl options, command, args = []
        result = ""
        Open3.popen3( "#{ config[ 'vsctl' ] } #{ options.join ' ' } #{ command } #{ args.join ' ' }" ) do | stdin, stdout, stderr |
          stdin.close
          error = stderr.read
          result << stdout.read
          raise "Permission denied #{ config[ 'vsctl' ] }" if /Permission denied/ =~ error
          raise "#{ error } #{ config[ 'vsctl' ] }" unless error.length == 0
        end
        result
      end

    end

  end

end
