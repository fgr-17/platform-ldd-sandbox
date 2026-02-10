#!/bin/bash

BASE_DIR="/workspace"
BUSYBOX_RISCV_PATH="/tmp/busybox-1.36.1/busybox"
BUILD_DIR="${BASE_DIR}/qemu/build"
INITRAMFS_PATH="${BUILD_DIR}/initramfs"
MODULES_SEARCH_PATH=(
    "${BASE_DIR}/platform-drv-dt"
    "${BASE_DIR}/platform-drv-dt-sysfs"
)

mkdir -p ${BUILD_DIR} && cd ${BUILD_DIR}
rm -f rootfs.cpio
rm -rf initramfs
mkdir -p initramfs/{bin,sbin,etc,proc,sys,usr/bin,usr/sbin,modules}

cp ${BUSYBOX_RISCV_PATH} initramfs/bin/
(cd initramfs/bin && ln -sf busybox sh)

echo "Including built modules..."
find ${MODULES_SEARCH_PATH[@]} -name "*.ko" -exec cp {} ${INITRAMFS_PATH}/modules/ \; 2>/dev/null
MODULE_COUNT=$(ls ${INITRAMFS_PATH}/modules/*.ko 2>/dev/null | wc -l)
echo "Found $MODULE_COUNT module(s)"

cat > ${INITRAMFS_PATH}/init << 'EOF'
#!/bin/sh
/bin/busybox --install -s /bin
mount -t proc none /proc
mount -t sysfs none /sys
mount -t devtmpfs none /dev

echo ""
echo "========================================"
echo "  Linux Device Driver Testing VM"
echo "========================================"
echo ""
echo "To exit: Ctrl+A then X"
echo ""

exec /bin/sh
EOF
chmod +x ${INITRAMFS_PATH}/init

# create the cpio from inside the directory so paths are relative (this is the important bit)
cd initramfs
# find . | cpio -H newc -o > ../rootfs.cpio
find . -print0 | cpio --null -o --format=newc | gzip > ${INITRAMFS_PATH}.cpio.gz
cd ..