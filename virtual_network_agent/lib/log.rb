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

require 'rubygems'
require 'singleton'
require 'webrick'

class Log < WEBrick::Log
  include Singleton

  def write message
    self << message
  end

  def initialize log_file = nil, level = nil
    @locker = Mutex::new
    super
  end

  def log level, data
    @locker.synchronize do
      super
    end
  end

  def log_file= ( log_file )
    @locker.synchronize do
      @log.close if @opened
      @log = open( log_file, "a+")
      @log.sync = true
      @opened = true
    end
  end

  def close
    @locker.synchronize do
      super
    end
  end

end
