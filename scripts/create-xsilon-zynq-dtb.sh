#!/bin/sh

if [ ! -e ./tmp ]; then
	if [ -e ./tmp-glibc ]; then
		echo "Create tmp -> tmp/glibc symbolic link"
		ln -sf tmp-glibc tmp
	fi
fi

DTC_EXE=./tmp/work-shared/han9250-qemuzynq/kernel-build-artifacts/scripts/dtc/dtc
DTS=/opt/poky-1.7/meta-xsilon/conf/machine/boards/han9250-qemuzynq/qemuzynq-han9250.dts
INCLUDE_PATH="-i /opt/poky-1.7/meta-xilinx/conf/machine/boards/common -i /opt/poky-1.7/meta-xilinx/conf/machine/boards/qemu -i /opt/poky-1.7/meta-xsilon/conf/machine/boards/han9250-zynq"
$DTC_EXE $INCLUDE_PATH -I dts -O dtb -o cache/xsilon-zynq.dtb $DTS
