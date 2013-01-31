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

module DB

  class Uri
    def initialize uri
      raise StandardError.new "Invalid uri (#{ uri })" unless %r,^(http|https)://[[:graph:]]+$, =~ uri
      @uri = uri
      @uri << '/' unless %r,/$, =~ @uri
    end

    def to_s
      @uri
    end

    def ==( other )
      other.to_s == @uri
    end

  end

end
