%w{ ruby rubygems ruby-dev }.each do | package_name |
  package package_name
end

gem_package "rubygems-update" do
  version "2.1.11"
  action :nothing
  subscribes :install, "package[rubygems]", :immediately
  notifies :run, "bash[update-rubygems]", :immediately
end

bash "update-rubygems" do
  user "root"
  code "update_rubygems _2.1.11_"
  action :nothing
  notifies :install, "gem_package[bundler]", :immediately
end

gem_package "bundler" do
  action :nothing
end
