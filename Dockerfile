FROM debian:stable-slim

# Install build tools, QEMU, kernel deps, device tree compiler, etc.
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        build-essential \
        bc \
        bison \
        flex \
        libssl-dev \
        libelf-dev \
        libncurses-dev \
        gcc \
        g++ \
        git \
        wget \
        curl \
        ca-certificates \
        qemu-system-misc \
        qemu-system-arm \
        qemu-system-riscv \
        device-tree-compiler \
        python3 \
        cpio \
        rsync \
        vim \
        dwarves \
        gcc-riscv64-linux-gnu \
        && \
    apt-get clean && rm -rf /var/lib/apt/lists/*


RUN printf "\nalias ls='ls --color=auto'\n" >> ~/.bashrc
RUN printf "\nalias ll='ls -alF'\n" >> ~/.bashrc

# Create a workspace. Mount it at runtime if you prefer bind-mounts.
WORKDIR /workspace

# Entry to a shell so you can run kernel builds and QEMU manually.
CMD ["/bin/bash"]
