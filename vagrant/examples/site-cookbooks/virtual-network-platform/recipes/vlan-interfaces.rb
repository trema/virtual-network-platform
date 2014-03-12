log 'install prerequisites for vlan interfaces... wait a few minutes.'
%w{ vlan ethtool }.each do | package_name |
  package package_name
end

directory "/home/vagrant/tmp" do
  owner "vagrant"
  group "vagrant"
  mode 0755
  action :create
end

template "/home/vagrant/tmp/vlan-interfaces" do
  source "vlan-interfaces.erb"
  mode 0644
  owner "vagrant"
  group "vagrant"
  variables :vlan_interfaces => node['vlan_interfaces']
end

bash 'vlan interfaces' do
  user "root"
  code <<-EOT
     cat /home/vagrant/tmp/vlan-interfaces >> /etc/network/interfaces
     /etc/init.d/networking restart
  EOT
  not_if "fgrep '# The vlan interfaces' /etc/network/interfaces"
end
