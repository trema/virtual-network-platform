package "ruby1.9.3" do
  notifies :run, "bash[update-rubygems]", :immediately
end

bash "update-rubygems" do
  user "root"
  code <<-'EOT'
    update-alternatives --set ruby /usr/bin/ruby1.9.1 &&
    update-alternatives --set gem /usr/bin/gem1.9.1 &&
    gem install rubygems-update &&
    update_rubygems
  EOT
  action :nothing
  notifies :install, "gem_package[bundler]", :immediately
end

gem_package "bundler" do
  action :nothing
end
