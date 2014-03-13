bash "create tun/tap devices" do
  user "root"
  code <<-'EOT'
    mkdir /dev/net &&
    mknod /dev/net/tun c 10 200 &&
    chmod 0666 /dev/net/tun
  EOT
  not_if { ::File.exists?("/dev/net/tun") }
end
