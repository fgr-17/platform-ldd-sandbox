#!/bin/bash
LINUX_DIR=/workspace/linux

cd $LINUX_DIR
make ARCH=riscv CROSS_COMPILE=riscv64-linux-gnu- defconfig
make ARCH=riscv CROSS_COMPILE=riscv64-linux-gnu- -j$(nproc)

# cd $LINUX_DIR/arch/riscv/boot/dts
# dtc -I dts -O dtb -o devicetree.dtb devicetree.dts

# make ARCH=riscv CROSS_COMPILE=riscv64-linux-gnu- uImage dtbs LOADADDR=0x00000000 -j$(nproc)
# make ARCH=riscv CROSS_COMPILE=riscv64-linux-gnu- -j$(nproc) modules
# make ARCH=riscv CROSS_COMPILE=riscv64-linux-gnu-  modules install 