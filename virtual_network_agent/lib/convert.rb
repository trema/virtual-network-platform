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

def convert_integer value
  return nil if value.nil?

  case value
    when Integer
      i = value
    when /^0[0-7]*$/
      i = value.oct
    when /^[1-9][[:digit:]]*$/
      i = value.to_i
    when /^0[Xx][[:xdigit:]]+$/
      i = value.hex
    else
      raise BadReuestError.new "Integer (#{ value }) is illegal format."
  end
  i
end

VNI_MAX = 0xffffff

def convert_vni value
  return nil if value.nil?

  begin
    i = convert_integer value
  rescue BadReuestError
    raise BadReuestError.new "Vni (#{ value }) is illegal format."
  end
  if i < 0 or i > VNI_MAX
    raise BadReuestError.new "Vni (#{ value }) is illegal range."
  end
  i
end

def convert_address value
  return nil if value.nil?

  case value
    when Integer
      a = []
      while a.length < 4
        a.unshift( "%u" % ( value & 0xff ) )
      end
      a.join( "." )
    when /^(?:(?:[[:digit:]]{1,3})\.){3}[[:digit:]]{1,3}$/
      a = []
      value.split(".").each do | each |
        i = each.to_i
        raise BadReuestError.new "Broadcast address (#{ value }) is illegal format." unless i < 256
        a.push( "%u" % i )
      end
      a.join( "." )
    else
      raise BadReuestError.new "Broadcast address (#{ value }) is illegal format."
  end
end

alias :convert_broadcast_address :convert_address

def convert_port value
  return nil if value.nil?

  begin
    i = convert_integer value
  rescue BadReuestError
    raise BadReuestError.new "Port number (#{ value }) is illegal format."
  end
  if i < 0 or i > 0xffff
    raise BadReuestError.new "Port number (#{ value }) is illegal range."
  end
  i
end
