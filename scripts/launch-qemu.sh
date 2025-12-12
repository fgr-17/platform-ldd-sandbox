#!/bin/bash

BASE_DIR="/workspace"
LINUX_BASE="${BASE_DIR}/linux"
QEMU_BUILD_PATH="${BASE_DIR}/qemu/build"

LINUX_KERNEL_IMAGE_PATH="${LINUX_BASE}/arch/riscv/boot/Image"
GENERATED_DTC_PATH="${LINUX_BASE}/scripts/dtc/dtc"

INITRAMFS_PATH="${QEMU_BUILD_PATH}/initramfs.cpio.gz"
DEVICETREE_PATH="${QEMU_BUILD_PATH}/qemu-riscv64"

./build-busybox.sh
./gen-initramfs.sh

extract_patched_dtb() {
    # extract patched DTB file (only if passing initrd)
    qemu-system-riscv64 \
    -machine virt,dumpdtb=${DEVICETREE_PATH}.dtb \
    -cpu rv64 \
    -m 512M \
    -nographic \
    -kernel ${LINUX_KERNEL_IMAGE_PATH} \
    -initrd ${INITRAMFS_PATH} \
    -append "console=ttyS0 earlycon=sbi" 

    # decompile dtb
    ${GENERATED_DTC_PATH} -I dtb -O dts ${DEVICETREE_PATH}.dtb > ${DEVICETREE_PATH}.dts
    # recompile dts
    ${GENERATED_DTC_PATH} -I dts -O dtb ${DEVICETREE_PATH}.dts -o ${DEVICETREE_PATH}.bak.dtb
}


run_qemu_with_dtb() {
    # run qemu with the extracted DTB file
    qemu-system-riscv64 \
    -machine virt \
    -cpu rv64 \
    -m 512M \
    -nographic \
    -kernel ${LINUX_KERNEL_IMAGE_PATH} \
    -initrd ${INITRAMFS_PATH} \
    -append "console=ttyS0 earlycon=sbi" \
    -dtb ${DEVICETREE_PATH}.dtb
}

case "${1:-}" in
    "extract")
        echo "Building prerequisites..."
        ./build-busybox.sh
        ./gen-initramfs.sh
        extract_patched_dtb
        ;;
    "run")
        echo "Building prerequisites..."
        ./build-busybox.sh
        ./gen-initramfs.sh
        run_qemu_with_dtb
        ;;
    "all")
        echo "Building prerequisites..."
        ./build-busybox.sh
        ./gen-initramfs.sh
        extract_patched_dtb
        run_qemu_with_dtb
        ;;
    "help"|"-h"|"--help")
        echo "Usage: $0 [COMMAND]"
        echo ""
        echo "Commands:"
        echo "  extract-dtb    Extract and patch DTB from QEMU"
        echo "  run           Run QEMU with existing DTB"
        echo "  all           Extract DTB and run QEMU (default)"
        echo "  help          Show this help"
        echo ""
        echo "If no command is specified, 'all' is used."
        ;;
    *)
        echo "Building prerequisites..."
        ./build-busybox.sh
        ./gen-initramfs.sh
        extract_patched_dtb
        run_qemu_with_dtb
        ;;
esac