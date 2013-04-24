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

require 'logger'
require 'singleton'

class Log < Logger
  include Singleton

  def initialize
    super( STDOUT )
    @level = INFO
    @formatter = Formatter.new
    @formatter.datetime_format = "%Y-%m-%d %H:%M:%S"
    @log_file = nil
  end

  def write message
    self << message
  end

  def log_file= ( log_file )
    @logdev.close unless @log_file.nil?
    @log_file = log_file
    if log_file.nil?
      log_file = STDOUT
    end
    @logdev = LogDevice.new( log_file, :shift_age => 0, :shift_size => 1048576 ) # TODO: read from the configuration settings
  end

  private

  class Formatter < Logger::Formatter
    def call( severity, time, progname, msg )
      "[%s.%06d] %-5s %s\n" % [ time.strftime('%Y-%m-%d %H:%M:%S'), time.usec.to_s, severity, msg2str(msg) ]
    end

  end

end
