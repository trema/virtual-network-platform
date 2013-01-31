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

class NetworkAgentError < StandardError
  attr_reader :code

  def initialize( message = "Internel error." )
    super( message )
    @code = 500
  end

end

class BadReuestError < NetworkAgentError
  def initialize( message = "Bad Request." )
    super( message )
    @code = 400
  end

end

class NotFoundError < NetworkAgentError
  def initialize( message = "Not found the specified network." )
    super( message )
    @code = 404
  end

end

class UnprocessableEntityError < NetworkAgentError
  def initialize( message = "The specified parameters are unacceptable." )
    super( message )
    @code = 422
  end

end

class NoOverlayNetworkFound < NotFoundError
  def initialize( vni, message = "No overlay network found (vni #{ vni } is not exists)." )
    super( message )
  end

end

class DuplicatedOverlayNetwork < UnprocessableEntityError
  def initialize( vni, message = "Failed to create a overlay network (duplicated vni #{ vni })." )
    super( message )
  end

end
