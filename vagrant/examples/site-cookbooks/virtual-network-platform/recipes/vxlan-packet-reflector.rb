log 'install prerequisites for vxlan packet reflector... wait a few minutes.'
%w{ gcc make }.each do | package_name |
  package package_name
end

log 'build vxlan packet reflector...'
bash "build vxlan packet reflector" do
  cwd "/home/vagrant/virtual-network-platform/vxlan_tunnel_endpoint/src"
  user "vagrant"
  code "make reflectord reflectorctl"
end

bash "setup init script" do
  user "root"
  code <<-'EOT'
    cp /home/vagrant/virtual-network-platform/vxlan_tunnel_endpoint/init/reflectord /etc/init.d
  EOT
  not_if { ::File.exists?("/etc/init.d/reflectord") }
end

bash "setup configuration files" do
  user "root"
  code <<-'EOT'
    cp /home/vagrant/virtual-network-platform/vxlan_tunnel_endpoint/config/reflectord /etc/default &&
    sed -i -e "s/^REFLECTORD_DIR=.*$/REFLECTORD_DIR=\"\/home\/vagrant\/virtual-network-platform\/vxlan_tunnel_endpoint\/src\"/" \
           -e "s/eth0/eth0.100/" \
        /etc/default/reflectord
  EOT
  not_if { ::File.exists?("/etc/default/reflectord") }
end

service "reflectord" do
  supports :status => true, :restart => true
  action [:enable, :start]
end
