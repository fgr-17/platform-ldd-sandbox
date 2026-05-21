MODULE_NAME := platform-drv-dt-lock
ARCH=riscv
CROSS_COMPILER=riscv64-linux-gnu-
KERNEL_DIR=/workspace/linux

obj-m := $(MODULE_NAME).o
$(MODULE_NAME)-objs := src/$(MODULE_NAME).o src/pcd-syscalls.o

BUILD_DIR := $(PWD)/build

all:
	@echo "Building $(MODULE_NAME) for kernel: $(KDIR)"
	@mkdir -p $(BUILD_DIR)
	make ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILER) -C $(KERNEL_DIR) M=$(PWD) modules
	@find . -maxdepth 2 \( -name '*.ko' -o -name '*.o' -o -name '*.mod' -o -name '*.mod.c' -o -name '.*.cmd' -o -name 'modules.order' -o -name 'Module.symvers' \) -exec mv {} $(BUILD_DIR)/ \; 2>/dev/null || true

clean:
	make ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILER) -C $(KERNEL_DIR) M=$(PWD) clean
help:
	make ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILER) -C $(KERNEL_DIR) M=$(PWD) help