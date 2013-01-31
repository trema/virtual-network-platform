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

class Configure
  include Singleton
  extend Forwardable

  def initialize
    config_file = File.dirname( __FILE__ ) + '/configure.yml'
    default_config = ( YAML.load_file( config_file ) or {} )
    if default_config.has_key? 'controller_uri'
      default_config[ 'controller_uri' ] << '/' unless %r,/$, =~ default_config[ 'controller_uri' ]
    end
    if default_config.has_key? 'uri'
      default_config[ 'uri' ] << '/' unless %r,/$, =~ default_config[ 'uri' ]
    end
    if default_config.has_key? 'datapath_id'
      case default_config[ 'datapath_id' ]
        when /^0[Xx][[:xdigit:]]+$/
          default_config[ 'datapath_id' ] = default_config[ 'datapath_id' ].hex
        else
          default_config[ 'datapath_id' ] = default_config[ 'datapath_id' ].to_i
      end
    end
    if default_config.has_key? 'reflector_mode'
      default_config[ 'reflector_mode' ] = ( default_config[ 'reflector_mode' ] == 'true' )
    else
      default_config[ 'reflector_mode' ] = false
    end
    if default_config.has_key? 'daemon'
      default_config[ 'daemon' ] = ( default_config[ 'daemon' ] == 'true' )
    else
      default_config[ 'daemon' ] = false
    end
    @config = default_config
  end

  def_delegator :@config, :[]
  def_delegator :@config, :[]=

  def to_hash
    @config
  end

end
