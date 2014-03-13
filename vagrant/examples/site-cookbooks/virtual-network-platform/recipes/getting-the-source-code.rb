package "git"

git "virtual-network-platform" do
  repository "https://github.com/trema/virtual-network-platform.git"
  destination "/home/vagrant/virtual-network-platform"
  revision "master"
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
