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

class NetworkManagementError < StandardError
  attr_reader :code

  def initialize( message = "Internel error." )
    super( message )
    @code = 500
  end

end

class BadReuestError < NetworkManagementError
  def initialize( message = "Bad Request." )
    super( message )
    @code = 400
  end

end

class NotFoundError < NetworkManagementError
  def initialize( message = "Not found the specified network." )
    super( message )
    @code = 404
  end

end

class UnprocessableEntityError < NetworkManagementError
  def initialize( message = "The specified parameters are unacceptable." )
    super( message )
    @code = 422
  end

end

class BusyHereError < NetworkManagementError
  def initialize( message = "Request is not allowed at this moment since an update is in progress." )
    super( message )
    @code = 486
  end
end

class NoSliceFound < NotFoundError
  def initialize( slice_id, message = "No slice found (slice #{ slice_id } is not exists)." )
    super( message )
  end

end

class NoPortFound < NotFoundError
  def initialize( port_id, message = "No port found (port #{ port_id } is not exists)." )
    super( message )
  end

end

class NoMacAddressFound < NotFoundError
  def initialize( mac_address, message = "No mac address found (mac address #{ mac_address } is not exists)." )
    super( message )
  end

end

class NoTargetFound < NetworkManagementError
  def initialize( message = "No target found." )
    super( message )
  end

end

class DuplicatedSlice < UnprocessableEntityError
  def initialize( slice_id, message = "Failed to create a slice (duplicated slice id #{ slice_id })." )
    super( message )
  end

end

class DuplicatedPortId < UnprocessableEntityError
  def initialize( port_id, message = "Failed to create a port (duplicated port id #{ port_id })." )
    super( message )
  end

end

class DuplicatedPort < UnprocessableEntityError
  def initialize( port, message = "Failed to create a port (duplicated port #{ port })." )
    super( message )
  end

end

class DuplicatedMacAddress < UnprocessableEntityError
  def initialize( mac_address, message = "Failed to create a mac address (duplicated mac address #{ mac_address })." )
    super( message )
  end

end
