#
# NOT WORK
#
# 1. Add a dependency on rbenv to virtual-network-platform/recipes/metadata.rb
#
#    depends 'rbenv'
#
# 2. Add a cookbook on rbenv to Cheffile
#
#    cookbook "rbenv"
#
# 3. Add a PATH to /etc/init.d/configuration_frontend
#
#    RBENV_ROOT=/opt/rbenv
#    PATH=$RBENV_ROOT/shims:$RBENV_ROOT/bin:/opt/rbenv/plugins/ruby_build/bin:$PATH
#
# 4. Add a PATH to /etc/init.d/virtual_network_agent
#
#    RBENV_ROOT=/opt/rbenv
#    PATH=$RBENV_ROOT/shims:$RBENV_ROOT/bin:/opt/rbenv/plugins/ruby_build/bin:$PATH
#
include_recipe "rbenv::default"
include_recipe "rbenv::ruby_build"

rbenv_ruby "2.1.1" do
  global true
  notifies :install, "rbenv_gem[bundler]", :immediately
end

rbenv_gem "bundler" do
  action :nothing
end
