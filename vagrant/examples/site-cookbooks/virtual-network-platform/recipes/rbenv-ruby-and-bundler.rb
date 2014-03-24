include_recipe "rbenv::default"
include_recipe "rbenv::ruby_build"

rbenv_ruby "2.1.1" do
  global true
  notifies :install, "rbenv_gem[bundler]", :immediately
end

rbenv_gem "bundler" do
  action :nothing
end

directory "/home/vagrant/tmp" do
  owner "vagrant"
  group "vagrant"
  mode 0755
  action :create
end

template "/home/vagrant/tmp/update-alternatives-ruby.sh" do
  source "update-alternatives-ruby.erb"
  mode 0755
  owner "vagrant"
  group "vagrant"
  variables :ruby_prefix => "/opt/rbenv/versions/2.1.1", :ruby_priority => "100"
end

bash 'update-alternatives install ruby' do
  user "root"
  code "sh /home/vagrant/tmp/update-alternatives-ruby.sh"
  not_if "update-alternatives --list ruby | fgrep 2.1.1"
end
