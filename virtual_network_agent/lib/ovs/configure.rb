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

require 'forwardable'
require 'singleton'
require 'yaml'

module OVS

  class Configure
    include Singleton
    extend Forwardable

    def initialize
      config_file = File.dirname( __FILE__ ) + '/configure.yml'
      default_config = ( YAML.load_file( config_file ) or {} )
      default_config[ 'ovs-vsctl' ] = which default_config[ 'ovs-vsctl' ]
      @config = default_config
    end

    def_delegator :@config, :[]
    def_delegator :@config, :[]=

    def to_hash
      @config
    end

    private

    def which command
      return command if %r,^/, =~ command
      ENV[ 'PATH' ].split( ':' ).each do | each |
	each << '/' unless %r,/$, =~ each
	path = each + command
        if File.executable_real?( path )
	  return path
	end
      end
      command
    end

  end

end
