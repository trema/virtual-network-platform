log 'install prerequisites for vxlan tunnel end-point... wait a few minutes.'
%w{ gcc make }.each do | package_name |
  package package_name
end

log 'build vxlan tunnel end-point...'
bash "build vxlan tunnel end-point" do
  cwd "/home/vagrant/virtual-network-platform/vxlan_tunnel_endpoint/src"
  user "vagrant"
  code "make"
end

bash "setup init script" do
  user "root"
  code <<-'EOT'
    cp /home/vagrant/virtual-network-platform/vxlan_tunnel_endpoint/init/vxland /etc/init.d
    update-rc.d vxland defaults
  EOT
  not_if { ::File.exists?("/etc/init.d/vxland") }
end

bash "setup configuration files" do
  user "root"
  code <<-'EOT'
    cp /home/vagrant/virtual-network-platform/vxlan_tunnel_endpoint/config/vxland /etc/default
    sed -i -e "s/^VXLAND_DIR=.*$/VXLAND_DIR=\"\/home\/vagrant\/virtual-network-platform\/vxlan_tunnel_endpoint\/src\"/" \
           -e "s/eth0/eth0.100/" \
        /etc/default/vxland
  EOT
  not_if { ::File.exists?("/etc/default/vxland") }
end

bash 'start vxlan tunnel end-point' do
  user "root"
  code "service vxland start"
end
