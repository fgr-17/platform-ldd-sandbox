#!/bin/bash
LINUX_DIR=/workspace/linux

cd $LINUX_DIR
make ARCH=riscv CROSS_COMPILE=riscv64-linux-gnu- defconfig

./scripts/config \
    --enable GPIOLIB \
    --enable GPIO_CDEV \
    --enable CONFIGFS_FS \
    --enable GPIO_SIM
make ARCH=riscv CROSS_COMPILE=riscv64-linux-gnu- olddefconfig

make ARCH=riscv CROSS_COMPILE=riscv64-linux-gnu- -j$(nproc)

# cd $LINUX_DIR/arch/riscv/boot/dts
# dtc -I dts -O dtb -o devicetree.dtb devicetree.dts

# make ARCH=riscv CROSS_COMPILE=riscv64-linux-gnu- uImage dtbs LOADADDR=0x00000000 -j$(nproc)
# make ARCH=riscv CROSS_COMPILE=riscv64-linux-gnu- -j$(nproc) modules
# make ARCH=riscv CROSS_COMPILE=riscv64-linux-gnu-  modules install 