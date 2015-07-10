#!/bin/sh

CWD=$(pwd)

KERNEL_WORK_DIR=./tmp/work/han9250_qemuzynq-oe-linux-gnueabi/linux-xsilon
HANADU_MOD_WORK_DIR=./tmp/work/han9250_qemuzynq-oe-linux-gnueabi/hanadu-mod

pushd $KERNEL_WORK_DIR/
KERNELS=(*/)
popd

pushd $HANADU_MOD_WORK_DIR/
HANADU_MODS=(*/)
popd

declare -p KERNELS
declare -p HANADU_MODS

if [ ${#KERNELS[@]} -gt 1 ]; then
	echo "There are ${#KERNELS[@]} kernel dirs in the current path";
	select dir in "${KERNELS[@]}"; do 
		echo "you selected ${dir}"'!';
		KERNEL=${dir}
		break; 
	done
else
	KERNEL=${KERNELS[0]}
fi

if [ ${#HANADU_MODS[@]} -gt 1 ]; then
	echo "There are ${#HANADU_MODS[@]} hanadu module dirs in the current path";
	select dir in "${HANADU_MODS[@]}"; do 
		echo "you selected ${dir}"'!';
		HANADU_MOD=${dir}
		break; 
	done
else
	HANADU_MOD=${HANADU_MODS[0]}
fi

HANADU_MOD_ABS_PATH=$CWD/$HANADU_MOD_WORK_DIR/$HANADU_MOD/git

echo "Adding $CWD/tmp/sysroots/x86_64-linux/usr/bin/arm-oe-linux-gnueabi to path"
PATH=$PATH:$CWD/tmp/sysroots/x86_64-linux/usr/bin/arm-oe-linux-gnueabi

echo "Starting GDB"
cd $KERNEL_WORK_DIR/$KERNEL/linux-han9250_qemuzynq-standard-build
echo "Creating .gdbinit in kernel build directory"
echo "add-auto-load-safe-path $(pwd)" > .gdbinit
echo "target remote :12001" >> .gdbinit
echo "b start_kernel" >> .gdbinit
echo "cont" >> .gdbinit

echo "lx-symbols $HANADU_MOD_ABS_PATH" > .gdbhanmod

arm-oe-linux-gnueabi-gdb vmlinux

# Go back to original directory.
cd $CWD

