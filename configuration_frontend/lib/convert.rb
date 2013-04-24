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
require 'db'

OFPP_MAX = 0xff00
OFPP_LOCAL = 0xfffe

OFP_MAX_PORT_NAME_LEN = 16

def convert_id value
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
      raise BadReuestError.new "Id (#{ value }) is illegal format."
  end
  i
end

SLICE_ID_MAX = 0xffffff

def convert_slice_id value
  return nil if value.nil?

  begin
    i = convert_id value
  rescue BadReuestError
    raise BadReuestError.new "Slice id (#{ value }) is illegal format."
  end
  if i < 0 or i > SLICE_ID_MAX
    raise BadReuestError.new "Slice id (#{ value }) is illegal range."
  end
  i
end

def convert_description value
  return "" if value.nil?
  return value if value.length < 2**16
  raise BadReuestError.new "Description is too long."
end

def convert_port_id value
  return nil if value.nil?

  begin
    convert_id value
  rescue BadReuestError
    raise BadReuestError.new "Port id (#{ value }) is illegal format."
  end
end

def convert_datapath_id value
  return nil if value.nil?

  begin
    i = DB::DatapathId.new value
  rescue => e
    raise BadReuestError.new e.message
  end
end

def convert_port_no value
  return DB::PORT_NO_UNDEFINED if value.nil?

  begin
    i = convert_id value
  rescue BadReuestError
    raise BadReuestError.new "Port number (#{ value }) is illegal format."
  end
  return i if i == OFPP_LOCAL
  return i if i >= 1 and i <= OFPP_MAX
  raise BadReuestError.new "Port number (#{ value }) is illegal range."
end

def convert_port_name value
  return "" if value.nil?

  unless /^\S/ =~ value
    raise BadReuestError.new "Port name ('#{ value }') is illegal format."
  end

  return value if value.length < OFP_MAX_PORT_NAME_LEN
  raise BadReuestError.new "Port name is too long."
end

def convert_vid value
  return DB::VLAN_ID_UNSPECIFIED if value.nil? or value == "none"

  begin
    i = convert_id value
  rescue BadReuestError
    raise BadReuestError.new "VLAN id (#{ value }) is illegal format."
  end
  return i if i == DB::VLAN_ID_UNSPECIFIED
  return i if i >= 0 and i <= 0x0fff
  raise BadReuestError.new "VLAN id (#{ value }) is illegal range."
end

def convert_mac value
  return nil if value.nil?

  begin
    i = DB::Mac.new value
  rescue => e
    raise BadReuestError.new e.message
  end
end

def convert_uri value
  return nil if value.nil?

  begin
    i = DB::Uri.new value
  rescue => e
    raise BadReuestError.new e.message
  end
end

def convert_tep value
  return nil if value.nil?

  begin
    i = DB::Tep.new value
  rescue => e
    raise BadReuestError.new e.message
  end
end
