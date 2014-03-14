package "ruby1.9.3" do
  notifies :run, "bash[update-rubygems]", :immediately
end

bash "update-rubygems" do
  user "root"
  code "gem update --system"
  action :nothing
end

gem_package "bundler"
