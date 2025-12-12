# Platform linux device drivers sandbox

Basic repo to play with platform devices, QEMU, devicetree, etc.

## Introduction

This repo is a sandbox to learn and play with linux platform device drivers and their connection with device-tree.
Unless you have a dedicated linux hardware you can play with, like a beaglebone or old rpi, it's always difficult to have a full-software playground where you can experiment and be sure cannot breaking anything (please don't break this repo).

So, the important terms here are:

* [docker](docker.com): provides a tailored environment to:
  * build the linux kernel image
  * build linux device drivers (through kbuild)
  * build busybox for target arch
  * compile device tree
  * launch qemu
* [QEMU](qemu.org): Open source emulator and virtualizer. Provides kernel isolation, lighter than a full featured VM
* [device-tree](https://www.devicetree.org): describes the hardware of the qemu system using text files (`.dts` and `.dtsi` files). Can be modified to add more devices to play with.
* [Linux platform buses, devices, drivers](https://docs.kernel.org/driver-api/driver-model/platform.html): provides infrastructure and software interfaces to interact with hardware that is not "hotpluggable" like USB devices, but are hardwired into the system.

~~~mermaid
graph LR

docker[Docker]
qemu[QEMU - kernel isolation]
dt[Device-Tree]
ldd[ldd]
lk[kernel image]
bbox[busybox]

lk -- builds --> ldd
docker -- builds --> lk
lk --> qemu
ldd -- runs on --> qemu
dt -- describes hw --> qemu
bbox -- cmds --> qemu
~~~


## Installation

This repo has submodules, you need to clone it recursively:

~~~bash
git clone --recurse-submodules <url>
~~~

... or, if already downloaded, run:

~~~bash
cd platform-ldd-sandbox
git submodule udpate --init
~~~

The submodule is the full linux repo, so be patient... it takes a while to download.

## Using the repo

### Create the environment

Use `compose.yml` to create the builder container and jump into it:

~~~bash
cd platform-ldd-sandbox
docker compose up -d
docker exec -it platform-ldd-sandbox bash
~~~

### Building linux kernel

Start by building the linux kernel for the risc-v arch (inside the `platform-ldd-sandbox` container):

~~~bash
cd /workspace/scripts
./build-linux-kernel.sh
~~~

If successfully finishes, should show something like this:

~~~bash
  Kernel: arch/riscv/boot/Image is ready
  GZIP    arch/riscv/boot/Image.gz
  Kernel: arch/riscv/boot/Image.gz is ready
~~~

First time will take several minutes, at least on my old PC. This kernel image should be build just once, at least with the workflow I been using.

### Building the driver

Once inside the container, goto the dir just run `make`

~~~bash
cd /workspace/platform-ldd-sandbox
make
~~~

Should see something like this:

~~~
root@e798879989b7:/workspace/platform-drv-dt# make
Building platform-drv-dt for kernel:
make ARCH=riscv CROSS_COMPILE=riscv64-linux-gnu- -C /workspace/linux M=/workspace/platform-drv-dt modules
make[1]: Entering directory '/workspace/linux'
  CC [M]  /workspace/platform-drv-dt/src/platform-drv-dt.o
  LD [M]  /workspace/platform-drv-dt/platform-drv-dt.o
  MODPOST /workspace/platform-drv-dt/Module.symvers
  CC [M]  /workspace/platform-drv-dt/platform-drv-dt.mod.o
  LD [M]  /workspace/platform-drv-dt/platform-drv-dt.ko
make[1]: Leaving directory '/workspace/linux'
~~~

If the driver was correctly build, you should see `platform-drv-dt/platform-drv-dt.ko`. That's what we need to play with.

**Note:** this [`Makefile`](./platform-drv-dt/Makefile) is not using gcc or any known compiler at all, but the `kbuild` linux system to build the module. If not familiar with that flow, you can check this other repo that explains better: [`ldd-sandbox`](https://github.com/fgr-17/ldd-sandbox)

For more specific info about the driver itself, please refer to [`/platform-drv-dt/README.md`](./platform-drv-dt/README.md)

### Testing the driver

Ok so, to test this, we need a couple of things first:

* Linux image: covered in [this section](#building-linux-kernel)
* The driver to test!: already covered in [the previous section](#building-the-driver)
* Kernel isolated environment: that's when QEMU comes into play
* Device-tree binary, that is a `.dtb` file compiled from the set of `.dts` and `.dtsi` device-tree source files
* Some CLI to talk to qemu: here we use BusyBox

So we need to put all of that together to run this command and launch qemu VM:

~~~bash
run_qemu_with_dtb() {
    qemu-system-riscv64 \
    -machine virt \
    -cpu rv64 \
    -m 512M \
    -nographic \
    -kernel ${LINUX_KERNEL_IMAGE_PATH} \
    -initrd ${INITRAMFS_PATH} \
    -append "console=ttyS0 earlycon=sbi" \
    -dtb ${DEVICETREE_PATH}.dtb
~~~

That's what's the [`launch-qemu.sh`](./scripts/launch-qemu.sh) script does.

You can start the qemu VM with the following command (**pass the `run` argument**):

~~~bash
root@e798879989b7:/workspace/scripts# ./launch-qemu.sh run
...
...
[    0.694373] debug_vm_pgtable: [debug_vm_pgtable         ]: Validating architecture page table helpers
[    0.703172] clk: Disabling unused clocks
[    0.703720] PM: genpd: Disabling unused power domains
[    0.704207] ALSA device list:
[    0.704610]   No soundcards found.
[    0.756412] Freeing unused kernel image (initmem) memory: 2248K
[    0.757481] Run /init as init process

========================================
  Linux Device Driver Testing VM
========================================

To exit: Ctrl+A then X

/bin/sh: can't access tty; job control turned off
~ #
~~~

Let's dig a little bit into the `initrd` and `dtb` args:

#### Building the initramfs

The filesystem where qemu starts is generated with the [`gen-initramfs.sh`](./scripts/gen-initramfs.sh) script
It has a minimum set of dirs to make this work:

~~~bash
initramfs
├── bin     # <<< busybox binary will be saved here
├── sbin
├── etc
├── proc
├── sys
├── usr/bin
└── modules  # <<< the driver will be stored here
~~~

1. The script called [`build-busybox.sh`](./scripts/build-busybox.sh) generates the BusyBox binary. This provides the set of commands we need to sail inside QEMU
2. Then, all the `*.ko` files that lives in the [`platform-drv-dt`](./platform-drv-dt/) dir will be copied to the `modules/` dir
3. An additional init script will be hardcoded from the script. That will be executed when qemu finishes loading the kernel
4. All that stuff is zipped into the final `initramfs.cpio.gz` that will be used by qemu

#### Compiling device-tree blob

What we need for this is the basic device tree of the qemu system we are using in this repo: `qemu-riscv64`, so we can modify afterwards with our own drivers.

The first problem here is that qemu doesn't provides a `.dts` file for this board, because it generates it on the fly. There's a version of the file we need in Zephyr repo, but I couldn't make it work. So the other option was to launch `qemu-riscv64` without any device-tree info, letting it auto generate its own stuff. Once inside, we can extract the `.dtb` file and decode our needed `.dts` file. That's what the [`extract-qemu-dts.sh`](./scripts/extract-qemu-dts.sh). After calling that script, you should have the original qemu `.dts` file, describing the internal hardware of this board.

**But, this repo also already has an modified version of this `.dts` file here: [`qemu-riscv64.dts`](./qemu/qemu-riscv64.dts)**. Instead of adding all the testing devices in that files, the best practice is to use "includes", so that file has only one change:

~~~C
/include/ "qemu-riscv64-pdev.dtsi"
~~~

After that, all the new devices can be added to the nodes of the original device-tree directly. There's also a script [`compile-dts.sh`](./scripts/compile-dts.sh) that generates the `.dtb` output using `dtc` compiler.

#### Shipping the kernel module into the `initramfs`

All the `*.ko` files are shipped into the `initramfs`, inside the `modules/` dir.

~~~bash
cd /workspace/scripts
./launch-qemu.sh run
~~~

Once qemu vm initializes:

~~~bash
~ # ls modules/
platform-drv-dt.ko
~~~

## License

See [LICENSE](LICENSE) file.

## Contributing

This is a learning project. Feel free to add more example drivers and submit pull requests!
