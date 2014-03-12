log 'install prerequisites for backend database... wait a few minutes.'
%w{ mysql-server mysql-client }.each do | package_name |
  package package_name
end

bash 'assign root password' do
  user "root"
  code "/usr/bin/mysqladmin -u root password root123"
  only_if "mysql -u root -e 'show databases;'"
end

bash 'add privileges to "root"' do
  user "root"
  code '/usr/bin/mysql --user=root --password=root123 -e \'grant all privileges on *.* to root@localhost identified by "root123" with grant option;\''
end

bash "create database and tables" do
  cwd "/home/vagrant/virtual-network-platform/backend_database"
  user "root"
  code "./create_database.sh"
end

if node['backend_database'] and node['backend_database']['reflector']
  reflector = node['backend_database']['reflector']
  bash "replace reflector" do
    user "root"
    code <<-EOT
      /usr/bin/mysql --user=root --password=root123 vnet -e \
	  'replace into reflectors (id, group_id, broadcast_address, broadcast_port, uri) \
	       values (#{reflector['id']}, #{reflector['group_id']}, \
		       \"#{reflector['broadcast_address']}\", #{reflector['broadcast_port']}, \
		       \"#{reflector['uri']}\");'
    EOT
  end
end
