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

begin
require 'rubygems'
rescue LoadError
end
require 'sinatra'

require 'configure'
require 'log'

class Daemon
  def self.start
      exit!( 0 ) if fork
      Process::setsid
      exit!( 0 ) if fork
      STDIN.reopen( "/dev/null" )
      STDOUT.reopen( "/dev/null", "w" )
      STDERR.reopen( "/dev/null", "w" )
      yield
  end
end

class WEBrickWrapper < Rack::Handler::WEBrick
 def self.run( app, options = {} )
   options[ :Logger ] = Log.instance
   options[ :AccessLog ] = [
     [ Log.instance, WEBrick::AccessLog::COMMON_LOG_FORMAT ],
     [ Log.instance, WEBrick::AccessLog::REFERER_LOG_FORMAT ]
   ]
   options[ :StartCallback ] = Proc.new {
     config = Configure.instance
     if not config[ 'pid_file' ].nil?
       File.open( config[ 'pid_file' ], 'w' ) do | file |
         file.puts Process.pid
       end
     end
   }
   options[ :StopCallback ] = Proc.new {
     config = Configure.instance
     if not config[ 'pid_file' ].nil?
       File.unlink( config[ 'pid_file' ] )
     end
   }
   config = Configure.instance
   if config[ 'daemon' ]
     options[ :ServerType ] = Daemon
   end
   super( app, options )
 end

end

Rack::Handler.register 'webrick_wrapper', 'WEBrickWrapper'
