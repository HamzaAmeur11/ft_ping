# Vagrantfile for ft_ping
#
# Subject requirements (Chapter III / VI):
#   - Debian >= 7.0
#   - Linux kernel > 3.14 (grading was originally designed on Debian 7.0 stable,
#     but any Debian release satisfying kernel > 3.14 is valid)
#   - Must be reachable/usable from another machine ("cluster computer") —
#     satisfied here via `vagrant ssh`
#
# Debian 12 (bookworm) is used instead of Debian 7 directly: it trivially
# satisfies ">= 7.0" and ships a kernel well above 3.14, without needing
# backports or a manual kernel upgrade on an EOL release.

Vagrant.configure("2") do |config|
  config.vm.box = "debian/bookworm64"
  config.vm.hostname = "ft-ping-vm"

  config.vm.provider "virtualbox" do |vb|
    vb.name   = "ft_ping"
    vb.memory = 1024
    vb.cpus   = 1
  end

  # Project sources are available inside the VM at this path
  config.vm.synced_folder ".", "/home/vagrant/ft_ping"

  config.vm.provision "shell", inline: <<-SHELL
    set -e

    apt-get update
    apt-get install -y \
      build-essential \
      gcc \
      make \
      valgrind \
      tcpdump \
      iproute2 \
      net-tools \
      iputils-ping

    echo "----------------------------------------"
    echo "Kernel version (must be > 3.14):"
    uname -r
    echo "Debian version:"
    cat /etc/debian_version
    echo "----------------------------------------"
  SHELL
end
