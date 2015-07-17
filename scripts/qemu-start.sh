#! /bin/sh

set -e

DEPLOY_DIR=/opt/poky-1.7/yocto-build/han9250-qemuzynq/debug/tmp/deploy/images/han9250-qemuzynq/


FLASH_DEV_SIZE_KB=$((64 * 1024))
SECTOR_SIZE_KB=64
FLASH_NUM_SECT=$(($FLASH_DEV_SIZE_KB / $SECTOR_SIZE_KB))

# @todo change to relative path once Xilinx QEMU is integrated into our build environment.
#QEMU_APP=~/ws/qemu/arm-softmmu/qemu-system-arm
#QEMU_APP=~/workspace/qemu/arm-softmmu/qemu-system-arm
QEMU_APP=./arm-softmmu/qemu-system-arm
#QEMU_APP=~/zynq_ws/qemu/arm-softmmu/qemu-system-arm
#QEMU_MAIN_OPTS=' --display none -smp 1 -machine xilinx-zynq-a9 -serial mon:stdio'
#QEMU_MAIN_OPTS='-M arm-generic-fdt -smp 2 -machine linux=on -chardev stdio,id=mon0 -mon chardev=mon0,mode=readline'
QEMU_MAIN_OPTS='-M arm-generic-fdt -smp 2 -machine linux=on --display none -serial mon:stdio'
SERVER_IP=10.4.18.4
ZYNQ_DTB_YOCTO=$DEPLOY_DIR/qemuzynq-han9250.dtb
ZYNQ_DTB_TEST=./cache/xsilon-zynq.dtb
ZYNQ_DTB=${ZYNQ_DTB_YOCTO}
ZYNQ_KERNEL=$DEPLOY_DIR/uImage
#ZYNQ_KERNEL=./tmp/work/han9250_zynq-oe-linux-gnueabi/linux-xsilon/4.0-brian+gitAUTOINC+133be0264f-r1/linux-han9250_zynq-standard-build/arch/arm/boot/zImage
#DEBUG_ARGS='-D qemu.log -d int'
DEBUG_ARGS=''
FLASH_ARGS=''

# $1 = MAC_ADDRESS
# $2 = Node Number
create_zynq_env()
{
	MACADDR_COLONS=${1//-/:}
	UBOOT_ENV_TEMPLATE=./scripts/uboot-env.txt
	FILENAME=$(basename "$UBOOT_ENV_TEMPLATE")
	UBOOT_ENV_FILE="cache/han9250-uboot-env-$1.txt"

	if [ ! -e cache ]; then
		mkdir cache;
	fi
	cp ${UBOOT_ENV_TEMPLATE} ${UBOOT_ENV_FILE}
	echo "Substituting ${MACADDR_COLONS} in ${UBOOT_ENV_FILE}"
	sed -i "s/!ETHADDR!/$MACADDR_COLONS/g" ${UBOOT_ENV_FILE}
	sed -i "s/!ETHADDRSPACES!/${MACADDR_COLONS//:/ }/g" ${UBOOT_ENV_FILE}
	sed -i "s/!ETHADDRPATH!/${MACADDR_COLONS//:/-}/g" ${UBOOT_ENV_FILE}
	sed -i "s/!NODENUM!/${2}/g" ${UBOOT_ENV_FILE}

	cp $UBOOT_ENV_FILE /export/han9250-zynq-rootfs-${MAC_ADDR}/mnt/boot/uEnv.txt
}


usage()
{
cat << EOF
usage: $0 options

Start the QEMU emulator

OPTIONS:
   -h      Show this message
   -n      The Node Number (Mandatory)
   -m      Last 3 octets of the MAC address e.g aa-00-01, if not set will use
           aa-nn-nn where nn-nn is the node number as a 16bit number. (Optional)
   -S      Suspend execution until debugger is attached. (Optional, default: off)
   -l      Log Level to pass to Kernel bootargs (Optional, default: 3)
   -s      Set NFS/TFTP Server IP (Optional, default: 10.4.18.4)
   -N      Set Network Simulator IP address  (Optional, default: 10.4.18.4)
   -P      Set Network Simulator UDP Port  (Optional, default: 11555)
   -u      Add USB Serial device (Optional, default: no)
   -t      Use test DTB (xsilon-zynq.dtb) within this directory.
           (Optional, default: use dtb in deploy directory)
   -a      Set AFE DIP Switch value (Optional, default: 0x04)
             (0x04) = Enable Hanadu Modem
             (0x02) = Enable 802.15.4 Radio
             (0x01) = Router Node
EOF
}

SUSPEND_QEMU=
LOGLEVEL=3
SERIAL_DEVICE_OPTS=""
NODE_NUM=0
NETSIM_ADDR=10.4.18.4
NETSIM_PORT=11555
AFE_DIPS="0x04"
while getopts “hl:m:n:s:tSuN:P:a:” OPTION; do
	case $OPTION in
	h)
		usage
		exit 1
		;;
	l)	
		LOGLEVEL=${OPTARG}
		;;
	m)	
		MAC_ADDR=${OPTARG}
		;;
	n)
		NODE_NUM=${OPTARG}
		;;
	S)
		SUSPEND_QEMU="-S -gdb tcp::12001"
		;;
	s)
		SERVER_IP=${OPTARG}
		;;
	t)
		ZYNQ_DTB=${ZYNQ_DTB_TEST}
		;;
	u)
		SERIAL_DEVICE_OPTS="-chardev pty,id=charserial0 -device usb-serial,chardev=charserial0,id=serial0,port=1"
		;;
	N)
		NETSIM_ADDR=${OPTARG}
		;;
	P)
		NETSIM_PORT=${OPTARG}
		;;
	a)
		AFE_DIPS=${OPTARG}
		;;
	?)
		usage
		exit
		;;
	esac
done

if [ $NODE_NUM -eq 0 ]; then
	echo "Must set node number"
	usage
	exit
fi
if [ -z ${MAC_ADDR} ]; then
	MAC_ADDR="aa-"$(printf "%.4x" $NODE_NUM |sed 's/^\([0-9A-Fa-f]\{2\}\)\([0-9A-Fa-f]\{2\}\).*$/\1-\2/')
fi

QEMU_BRIDGE_NET_OPTS="-netdev bridge,helper=/usr/local/libexec/qemu-bridge-helper,id=hn0 -net nic,macaddr=00:03:9a:${MAC_ADDR//-/:},model=cadence_gem,name=nic1,netdev=hn0"
XSILON_ARGS="-xsilon dipafe=${AFE_DIPS},nsaddr=${NETSIM_ADDR},nsport=${NETSIM_PORT}"

create_zynq_env ${MAC_ADDR} ${NODE_NUM}
MTDPARTS="mtdparts=e2000000.flash:128k(env),128k(uboot),48m(rootfs),16,128k(app)"
ZYNQ_KERNEL_ARGS_NFS="console=ttyPS0,115200 earlyprintk eth=00:03:9a:${MAC_ADDR//-/:} ip=:::::eth0:dhcp rootfstype=nfs root=/dev/nfs rw nfsroot=${SERVER_IP}:/export/han9250-zynq-rootfs-${MAC_ADDR},tcp,nolock,wsize=4096,rsize=4096 loglevel=${LOGLEVEL}"
#ZYNQ_KERNEL_ARGS_NFS="console=ttyPS0,115200 earlyprintk eth=00:03:9a:${MAC_ADDR//-/:} ip=:::::eth0:dhcp rootfstype=nfs root=/dev/nfs rw nfsroot=${SERVER_IP}:/export/han9250-zynq-rootfs-${MAC_ADDR},tcp,nolock,wsize=4096,rsize=4096 ${MTDPARTS} loglevel=${LOGLEVEL}"
ZYNQ_KERNEL_ARGS_RAM="console=ttyPS0,115200 earlyprintk ip=:::::eth0:dhcp root=/dev/ram ${MTDPARTS} loglevel=${LOGLEVEL}"

# Start the emulator
#-drive file=riscos-2014-06-04-RC12a.img,if=sd
#-sd sdimage.bin
CMD="${QEMU_APP} ${QEMU_MAIN_OPTS} ${XSILON_ARGS} ${FLASH_ARGS} ${DEBUG_ARGS} -dtb ${ZYNQ_DTB} -kernel ${ZYNQ_KERNEL} ${SUSPEND_QEMU} ${QEMU_BRIDGE_NET_OPTS} ${SERIAL_DEVICE_OPTS} -append \"$ZYNQ_KERNEL_ARGS_NFS\""
echo "Running ${CMD}"
eval ${CMD}

exit 0

