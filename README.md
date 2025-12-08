# Platform linux device drivers sandbox

Basic repo to play with platform devices, QEMU, devicetree, etc.

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

Use `compose.yml` to create the builder container and jump into it:

~~~bash
cd platform-ldd-sandbox
docker compose up -d
docker exec -it platform-ldd-sandbox bash
~~~

Start by building the linux kernel for the risc-v arch (inside the container):

~~~bash
cd /workspace/scripts
./build.sh
~~~

If successfully finishes, should show something like this:

~~~bash
  Kernel: arch/riscv/boot/Image is ready
  GZIP    arch/riscv/boot/Image.gz
  Kernel: arch/riscv/boot/Image.gz is ready
~~~

