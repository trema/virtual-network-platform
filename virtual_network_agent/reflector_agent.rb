#!/usr/bin/env ruby
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

required_path = File.dirname( __FILE__ ) + '/lib'
$LOAD_PATH.unshift required_path unless $LOAD_PATH.include? required_path

require 'optparse'
begin
require 'rubygems'
rescue LoadError
end
require 'json'
require 'sinatra/base'

require 'errors'
require 'configure'
require 'log'
require 'ovs'
require 'reflector'
require 'vxlan'
require 'webrick_wrapper'

def puts( msg )
  if Configure.instance[ :run_background ]
    Log.instance.info( msg )
  else
    super
  end
end

class ReflectorAgent < Sinatra::Base

  set :server, [ 'webrick_wrapper' ]

  logger = Log.instance
  OVS::Log.logger = logger
  Vxlan::Log.logger = logger
  use Rack::CommonLogger, logger

  def json_parse request, requires = []
    logger = Log.instance
    request.body.rewind
    body = request.body.read.gsub( /\000.*$/, '' ) # XXX
    begin
      parameters = JSON.parse( body, :symbolize_names => true )
    rescue => e
      logger.debug "body = '#{ body }'"
      raise BadReuestError.new e.message
    end
    requires.each do | each |
      raise BadReuestError.new unless parameters.has_key? each
    end
    parameters
  end

  def json_generate response
    JSON.pretty_generate( response ) << "\n"
  end

  def no_message_body response
    ""
  end

  # Error

  #set :dump_errors, false
  set :show_exceptions, false

  error do
    e = env[ 'sinatra.error' ]
    content_type 'text/plain'
    status e.code if e.kind_of?( NetworkAgentError )
    e.message + "\n"
  end

  # Debug

  before do
    logger.debug "url='#{ request.path_info }'"
    request.body.rewind
    logger.debug "body='#{ request.body.read }'"
  end

  after do
    logger.debug "status=#{ status }"
    logger.debug "body='#{ body }'"
  end

  # Reflector

  get '/reflector/?' do
    status 405
  end

  post '/reflector/?' do
    status 405
  end

  put '/reflector/?' do
    status 405
  end

  delete '/reflector/?' do
    status 405
  end

  #

  get '/reflector/:vni/?' do | vni |
    logger.debug "#{ __FILE__ }:#{ __LINE__ }: List summary of tunnel endpoint associated."
    parameters = { :vni => vni }
    content_type :json, :charset => 'utf-8'
    status 202
    body json_generate Reflector.list_endpoints( parameters )
  end

  post '/reflector/:vni/?' do | vni |
    logger.debug "#{ __FILE__ }:#{ __LINE__ }: Create a tunnel endpoint on the vni in the request URI."
    requires = [ :ip, :port ]
    parameters = json_parse( request, requires )
    parameters[ :vni ] = vni
    content_type :json, :charset => 'utf-8'
    status 202
    body no_message_body Reflector.add_endpoint( parameters )
  end

  put '/reflector/:vni/?' do | vni |
    status 405
  end

  delete '/reflector/:vni/?' do | vni |
    status 405
  end

  #

  get '/reflector/:vni/:tunnel_endpoint/?' do | vni, tunnel_endpoint |
    status 405
  end

  post '/reflector/:vni/:tunnel_endpoint/?' do | vni, tunnel_endpoint |
    status 405
  end

  put '/reflector/:vni/:tunnel_endpoint/?' do | vni, tunnel_endpoint |
    status 405
  end

  delete '/reflector/:vni/:tunnel_endpoint/?' do | vni, tunnel_endpoint |
    logger.debug "#{ __FILE__ }:#{ __LINE__ }: Remove a tunnel endpoint from the vni."
    parameters = { :vni => vni, :ip => tunnel_endpoint }
    content_type :json, :charset => 'utf-8'
    status 202
    body no_message_body Reflector.delete_endpoint( parameters )
  end

  #

  get '/*' do
    status 404
  end

  post '/*' do
    status 404
  end

  put '/*' do
    status 404
  end

  delete '/*' do
    status 404
  end

  config = Configure.instance
  config_file = File.dirname( __FILE__ ) + '/configure.yml'
  if File.readable? config_file
    config.load_file( config_file )
  end
  config_file = File.dirname( __FILE__ ) + '/reflector_configure.yml'
  config.load_file( config_file )
  option = OptionParser.new
  option.on( '-c arg', '--config-file=arg', "Configuration file (default #{ config_file })" ) do | arg |
    config.load_file arg
  end

  option.on( '--controller_uri=arg', "Controller uri (default '#{ config[ 'controller_uri' ] }')" ) do | arg |
    config[ 'controller_uri' ] = arg
  end
  option.on( '--[no-]pid-file=arg', "Pid file (default '#{ config[ 'pid_file' ] }')" ) do | arg |
    if arg
      config[ 'pid_file' ] = arg
    else
      config[ 'pid_file' ] = nil
    end
  end
  option.on( '--[no-]log-file=arg', "Log file (default '#{ config[ 'log_file' ] }')" ) do | arg |
    if arg
      config[ 'log_file' ] = arg
    else
      config[ 'log_file' ] = nil
    end
  end
  option.on( '--log-file-count=arg', "Log file count (default '#{ config[ 'log_file_count' ] }')" ) do | arg |
    config[ 'log_file_count' ] = arg
  end
  option.on( '--log-file-size=arg', "Log file size (default '#{ config[ 'log_file_size' ] }')" ) do | arg |
    config[ 'log_file_size' ] = arg
  end
  option.on( '-b', '--[no-]daemon', "Daemonize (default '#{ config[ 'daemon' ] }')" ) do | arg |
    config[ 'daemon' ] = arg
  end
  option.on( '--address=arg', "Listen address (default '#{ config[ 'listen_address' ] }')" ) do | arg |
    config[ 'listen_address' ] = arg
  end
  option.on( '--port=arg', "Listen port number (default '#{ config[ 'listen_port' ] }')" ) do | arg |
    config[ 'listen_port' ] = arg
  end
  option.on( '--ovs-vsctl=arg', "Path for ovs-vsctl (default: '#{ OVS::Configure.instance[ 'vsctl' ] }')" ) do | arg |
    OVS::Configure.instance[ 'vsctl' ] = arg
  end
  option.on( '-v', '--verbose', "Verbose mode" ) do | arg |
    Log.instance.level = Log::DEBUG
  end
  option.on( '-d', '--debug', "Debug mode (Identical to --no-pid-file --no-log-file --no-daemon --verbose)" ) do | arg |
    config[ 'pid_file' ] = nil
    config[ 'log_file' ] = nil
    config[ 'daemon' ] = false
    Log.instance.level = Log::DEBUG
  end
  option.on( '--help', "Show this message" ) do | arg |
    puts option.help
    exit 0
  end
  begin
    option.parse! ARGV
  rescue OptionParser::ParseError => e
    puts e.message
    puts option.help
    exit 1
  end
  if ARGV.size != 0
    puts option.help
    exit 1
  end
  logger.shift_age = config[ 'log_file_count' ].to_i unless config[ 'log_file_count' ].nil?
  logger.shift_size = config[ 'log_file_size' ].to_i unless config[ 'log_file_size' ].nil?
  logger.log_file = config[ 'log_file' ] unless config[ 'log_file' ].nil?
  set :port, config[ 'listen_port' ]
  set :bind, config[ 'listen_address' ]
  config[ :startup_callback ] = Reflector
  config[ :run_background ] = config[ 'daemon' ]

  logger.info "#{ self.name }"

  run! if __FILE__ == $0

end
