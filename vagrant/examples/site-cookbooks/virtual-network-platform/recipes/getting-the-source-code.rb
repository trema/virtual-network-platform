package "git"

git "virtual-network-platform" do
  repository "https://github.com/trema/virtual-network-platform.git"
  destination "/home/vagrant/virtual-network-platform"
  revision "feature/activerecord_3.2"
  action :sync
  user "vagrant"
  group "vagrant"
end

bash "git submodule update" do
  cwd "/home/vagrant/virtual-network-platform"
  user "vagrant"
  code <<-'EOT'
    git config submodule.trema.url https://github.com/trema/trema.git &&
    git submodule init &&
    git submodule update
  EOT
end

file "/home/vagrant/virtual-network-platform/trema/.ruby-version" do
  action :delete
  only_if {::File.exists?("/home/vagrant/virtual-network-platform/trema/.ruby-version")}
end
