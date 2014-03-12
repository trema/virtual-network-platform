execute "mkdir /dev/net; mknod /dev/net/tun c 10 200; chmod 0666 /dev/net/tun" do
  not_if { ::File.exists?("/dev/net/tun") }
end
