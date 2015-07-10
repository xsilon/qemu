#! /bin/sh

set -e

# Make sure only root can run our script
if [[ $EUID -ne 0 ]]; then
   echo "This script must be run as root" 1>&2
   exit 1
fi

usage()
{
cat << EOF
usage: $0 options

Populate the TFTP server with the linux and device tree binary for a given
board type. 

OPTIONS:
   -h      Show this message
   -n      Node Serial Number (on label on back of unit)
   -d      Override deploy directory that contains the linux and DTB images.
EOF
}

QEMU=0
BOARD=han9250
while getopts “hn:d:q” OPTION; do
	case $OPTION in
	h)
		usage
		exit 1
		;;
	n)
		NODENUM=${OPTARG}
		;;	
	d)
		DEPLOY_DIR=${OPTARG}
		;;
	?)
		usage
		exit
		;;
	esac
done

if [ -z ${BOARD} ]; then
	echo "You must set the board type"
	usage
	exit 1
fi

if [ -z $NODENUM ]
then
	echo "Please provide a valid Node Number in decimal (1-65535)"
	usage
	exit 1
fi
# ensure in decimal format
NODENUM=$((10#$NODENUM))

MAC_ADDR="aa-"$(printf "%.4x" $NODENUM |sed 's/^\([0-9A-Fa-f]\{2\}\)\([0-9A-Fa-f]\{2\}\).*$/\1-\2/')
ROOTFS_FILENAME=core-image-minimal-${BOARD}-qemuzynq.tar.gz
if [ -z ${DEPLOY_DIR} ]; then
	DEPLOY_DIR=tmp/deploy/images/${BOARD}-qemuzynq
fi
NFSDIR=/export/han9250-zynq-rootfs-${MAC_ADDR}

if [ ! -e ${NFSDIR} ]; then
	echo "NFS directory (${NFSDIR}) doesn't exist, creating..."
	mkdir -p ${NFSDIR}
else
	echo "Removing old contents from NFS directory (${NFSDIR})"
	rm -Rf ${NFSDIR}/*
fi

echo "Uncompressing tar.gz file $ROOTFS into ${NFSDIR}"
tar -zxf ${DEPLOY_DIR}/${ROOTFS_FILENAME} -C ${NFSDIR}
chmod 777 ${NFSDIR}/mnt/boot
touch ${NFSDIR}/mnt/boot/qemu


