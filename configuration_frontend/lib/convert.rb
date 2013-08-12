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
      raise BadRequestError.new "Id (#{ value }) is illegal format."
  end
  i
end

SLICE_ID_MIN = 0
SLICE_ID_MAX = 0xffffff

def convert_slice_id value
  return nil if value.nil?

  begin
    id = convert_id value
  rescue BadRequestError
    raise BadRequestError.new "Slice id (#{ value }) is illegal format."
  end
  if id < SLICE_ID_MIN or id > SLICE_ID_MAX
    raise BadRequestError.new "Slice id (#{ value }) is illegal range."
  end
  id
end

MAX_DESCRIPTION_LEN = 0xffff

def convert_description value
  return "" if value.nil?

  if value.length > MAX_DESCRIPTION_LEN
    raise BadRequestError.new "Description ('#{ value[ 0 .. 15 ] }...') is too long."
  end
  value
end

PORT_ID_MIN = 1
PORT_ID_MAX = 0xffffffff

def convert_port_id value
  return nil if value.nil?

  begin
    id = convert_id value
  rescue BadRequestError
    raise BadRequestError.new "Port id (#{ value }) is illegal format."
  end
  if id < PORT_ID_MIN or id > PORT_ID_MAX
    raise BadRequestError.new "Port id (#{ value }) is illegal range."
  end
  id
end

def convert_datapath_id value
  return nil if value.nil?

  begin
    DB::DatapathId.new value
  rescue => e
    raise BadRequestError.new e.message
  end
end

def convert_port_no value
  return DB::PORT_NO_UNDEFINED if value.nil?

  begin
    no = convert_id value
  rescue BadRequestError
    raise BadRequestError.new "Port number (#{ value }) is illegal format."
  end
  if ( no < 1 or no > OFPP_MAX ) and no != OFPP_LOCAL
    raise BadRequestError.new "Port number (#{ value }) is illegal range."
  end
  no
end

def convert_port_name value
  return "" if value.nil?

  unless /^\S/ =~ value
    raise BadRequestError.new "Port name ('#{ value }') is illegal format."
  end
  if value.length >= OFP_MAX_PORT_NAME_LEN
    raise BadRequestError.new "Port name ('#{ value[ 0 .. 15 ] }...') is too long."
  end
  value
end

# VLAN ID's 0 and 4095 (0x0fff) cannot be used meybe, because reserved by other system
VLAN_ID_MIN = 0
VLAN_ID_MAX = 0x0fff

def convert_vid value
  return DB::VLAN_ID_UNSPECIFIED if value.nil? or value == "none"

  begin
    vid = convert_id value
  rescue BadRequestError
    raise BadRequestError.new "VLAN id (#{ value }) is illegal format."
  end
  if ( vid < VLAN_ID_MIN or vid > VLAN_ID_MAX ) and vid != DB::VLAN_ID_UNSPECIFIED
    raise BadRequestError.new "VLAN id (#{ value }) is illegal range."
  end
  vid
end

def convert_mac value
  return nil if value.nil?

  begin
    DB::Mac.new value
  rescue => e
    raise BadRequestError.new e.message
  end
end

def convert_uri value
  return nil if value.nil?

  begin
    DB::Uri.new value
  rescue => e
    raise BadRequestError.new e.message
  end
end

def convert_tep value
  return nil if value.nil?

  begin
    DB::Tep.new value
  rescue => e
    raise BadRequestError.new e.message
  end
end
