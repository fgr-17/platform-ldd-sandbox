#!/bin/bash
BUSYBOX_VER="busybox-1.36.1"

cd /tmp

if [ -x "${BUSYBOX_VER}/busybox" ]; then
    echo "BusyBox binary already exists, skipping build..."
    exit 0
fi

wget https://busybox.net/downloads/${BUSYBOX_VER}.tar.bz2
tar xf ${BUSYBOX_VER}.tar.bz2
cd ${BUSYBOX_VER}
make CROSS_COMPILE=riscv64-linux-gnu- defconfig
sed -i 's/# CONFIG_STATIC is not set/CONFIG_STATIC=y/' .config
sed -i 's/CONFIG_TC=y/# CONFIG_TC is not set/' .config
make CROSS_COMPILE=riscv64-linux-gnu- -j$(nproc)