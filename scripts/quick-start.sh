#!/bin/sh

usage()
{
cat << EOF
usage: $0 options

Start the QEMU emulator

OPTIONS:
   -h      Show this message
   -n      The Node Number (Mandatory)
   -s      Start Debugger
EOF
}

SUSPEND_QEMU=
DEBUG_OPTS=
NODE_NUM=0
while getopts “hl:m:n:s:tSuN:P:a:” OPTION; do
	case $OPTION in
	h)
		usage
		exit 1
		;;
	n)
		NODE_NUM=${OPTARG}
		;;
	s)
		DEBUG_OPTS=
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

./scripts/qemu-start.sh -l 20 -s 10.4.18.104 -m aa-00-01 -n 1 -t -N 10.4.18.104 -P 11555

