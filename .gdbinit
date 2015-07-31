#SIG64 SIG63 SIG62
handle SIGUSR1 noprint nostop

run -D -d /tmp/qemu-node-1.log -M arm-generic-fdt -smp 2 -machine linux=on --display none -serial mon:stdio -xsilon dipafe=0x04,nsaddr=10.4.18.104,nsport=11555 -dtb ./cache/xsilon-zynq.dtb -kernel /opt/poky-1.7/yocto-build/han9250-qemuzynq/debug/tmp/deploy/images/han9250-qemuzynq//uImage -netdev bridge,helper=/usr/local/libexec/qemu-bridge-helper,id=hn0 -net nic,macaddr=00:03:9a:aa:00:01,model=cadence_gem,name=nic1,netdev=hn0 -append "console=ttyPS0,115200 earlyprintk eth=00:03:9a:aa:00:01 ip=:::::eth0:dhcp rootfstype=nfs root=/dev/nfs rw nfsroot=10.4.18.104:/export/han9250-zynq-rootfs-aa-00-01,tcp,nolock,wsize=4096,rsize=4096 loglevel=20"


