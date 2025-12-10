#!/bin/bash
# this script is currently replaced by `launch-qemu.sh` that contains an "extract" function
# The extracted dtb file in this case only works if not passing initrd param when launching
# qemu, so qemu patches the dtb with initrd info and addresses. The extracted dtb here doesn't 
# contain a section that is needed when passing dtb as a param, so qemu doesn't take care
# of the patching process
OUTPUT_FILE="qemu-riscv64"
GENERATED_DTC_PATH="/workspace/linux/scripts/dtc/dtc"
OUTPUT_PATH="/workspace/qemu"

qemu-system-riscv64 -machine virt,dumpdtb=${OUTPUT_FILE}.dtb
${GENERATED_DTC_PATH} -I dtb -O dts ${OUTPUT_FILE}.dtb > ${OUTPUT_FILE}.dts
mv ${OUTPUT_FILE}.dtb ${OUTPUT_PATH}/build/${OUTPUT_FILE}.dtb 
mv ${OUTPUT_FILE}.dts ${OUTPUT_PATH}/${OUTPUT_FILE}.dts