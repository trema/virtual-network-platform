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
require 'vxlan/linux_kernel'
require 'vxlan/vxlan_tunnel_endpoint'

module Vxlan

  class Instance
    class << self
      def add vni, address = nil
        adapter.add vni, address
      end

      def delete vni
        adapter.delete vni
      end

      def name vni
        adapter.name vni
      end

      def list
        adapter.list
      end

      def exists? vni
        adapter.exists? vni
      end

      private

      def adapter
	case Configure.instance[ 'adapter' ]
	  when 'linux_kernel'
	    LinuxKernel::Instance
	  else
            VxlanTunnelEndpoint::Instance
	end
      end

    end

  end

end
