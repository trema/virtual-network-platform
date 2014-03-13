log 'install prerequisites for configuration frontend... wait a few minutes.'
%w{ ruby ruby-json ruby-sinatra ruby-activerecord ruby-mysql }.each do | package_name |
  package package_name
end

bash "setup init script" do
  user "root"
  code <<-'EOT'
    cp /home/vagrant/virtual-network-platform/configuration_frontend/init/configuration_frontend /etc/init.d
  EOT
  not_if { ::File.exists?("/etc/init.d/configuration_frontend") }
end

bash "setup configuration files" do
  user "root"
  code <<-'EOT'
    cp /home/vagrant/virtual-network-platform/configuration_frontend/config/configuration_frontend /etc/default &&
    sed -i -e "s/^CONFIGURATION_FRONTEND_DIR=.*$/CONFIGURATION_FRONTEND_DIR=\"\/home\/vagrant\/virtual-network-platform\/configuration_frontend\"/" /etc/default/configuration_frontend
  EOT
  not_if { ::File.exists?("/etc/default/configuration_frontend") }
end

service "configuration_frontend" do
  supports :status => true, :restart => true
  action [:enable, :start]
end
