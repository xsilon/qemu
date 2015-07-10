#!/bin/sh

# Make sure only root can run our script
if [[ $EUID -ne 0 ]]; then
   echo "This script must be run as root" 1>&2
   exit 1
fi

usage()
{
cat << EOF
usage: $0 options

Setup the bridge br0 for using the bridge backend in QEMU.

EOF
}

ifconfig br0 down
brctl delbr br0

set -e 

ifconfig eth0 0.0.0.0 promisc up
 
brctl addbr br0
brctl setfd br0 0
brctl addif br0 eth0

set +e
 
echo 1 > /proc/sys/net/ipv4/conf/br0/proxy_arp
echo 1 > /proc/sys/net/ipv4/conf/eth0/proxy_arp
echo 1 > /proc/sys/net/ipv4/ip_forward
 
dhclient br0
ifconfig br0 up

