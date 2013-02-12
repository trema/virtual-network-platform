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
require 'ovs/log'

module OVS

  class Bridge
    class << self
      def list
        Vsctl.list_br
      end

      def datapath_id
        if @datapath_id.nil?
          @datapath_id = Vsctl.datapath_id
        end
        @datapath_id
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

  class Controller
    class << self
      def is_connected?
        Vsctl.is_connected?
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

      def datapath_id
        s = ovs_vsctl( [ '--timeout=1' ], 'get', [ 'bridge', bridge, 'datapath_id' ] ).split( "\n").first
        if /^"([[:xdigit:]]+)"$/ =~ s
          $1.hex
        else
          raise "Can not get datapath id"
        end
      end

      def is_connected?
        s = ovs_vsctl( [ '--timeout=1' ], 'get', [ 'controller', bridge, 'is_connected' ] )
        /true/ =~ s and true or false
      end

      private

      def config
        Configure.instance
      end

      def bridge
        config[ 'bridge' ]
      end

      def ovs_vsctl options, command, args = []
        command_args = "#{ config[ 'vsctl' ] } #{ options.join ' ' } #{ command } #{ args.join ' ' }"
        logger.debug "ovs_vsctl: '#{ command_args }'"
        result = ""
        Open3.popen3( command_args ) do | stdin, stdout, stderr |
          stdin.close
          error = stderr.read
          result << stdout.read
          raise "Permission denied #{ config[ 'vsctl' ] }" if /Permission denied/ =~ error
          raise "#{ error } #{ config[ 'vsctl' ] }" unless error.length == 0
        end
        result
      end

      def logger
        Log.instance
      end

    end

  end

end
