log 'install prerequisites for backend database... wait a few minutes.'
%w{ mysql-server mysql-client }.each do | package_name |
  package package_name
end

bash 'assign root password' do
  user "root"
  code "/usr/bin/mysqladmin -u root password root123"
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
