# Default to latest Ubuntu LTS.
# (See https://hub.docker.com/r/microsoft/devcontainers-base)
ARG VARIANT="ubuntu"
ARG PLATFORM="linux/amd64"
FROM --platform=${PLATFORM} mcr.microsoft.com/devcontainers/base:${VARIANT}

# From https://github.com/chipsalliance/verible/releases
ARG VERIBLE_VERSION="v0.0-4007-g98bdb38a"

# From https://github.com/verilator/verilator/releases
ARG VERILATOR_VERSION="v5.044"

# Cache large downloads:

# Clone Verilator (built from source)
RUN cd /tmp && \
    git clone --branch ${VERILATOR_VERSION} --depth 1 --single-branch https://github.com/verilator/verilator.git

    # Clone Verible (download from chipsalliance)
ARG VERIBLE_ARCHIVE="verible-${VERIBLE_VERSION}-linux-static-x86_64.tar.gz"
RUN wget https://github.com/chipsalliance/verible/releases/download/${VERIBLE_VERSION}/${VERIBLE_ARCHIVE} -P /tmp

# Install apt package dependencies.
#
# Upgrade:
#
#   ca-certificates
#
# For VS Code "xaver.clang-format" extension:
# 
#   clang-format
#
# GtkWave:
#
#   gtkwave
#
# Verilog-HDL/SystemVerilog/Bluespec SystemVerilog extension dependencies
#
#   universal-ctags
#   verilator
#
# Verilator:
#
#   autoconf
#   bison
#   build-essential
#   flex
#   help2man
#   libfl-dev
#   zlib1g-dev     (for FST waveform support)
#
# cocotb:
#
#   python3
#   python3-dev    (for embedding Python in simulator processes)
#   python3-pip
#   python3-venv

# Use unattended mode for 'apt'
ARG DEBIAN_FRONTEND="noninteractive"

RUN apt-get update && apt-get -y install --no-install-recommends \
    autoconf \
    bison \
    build-essential \
    ca-certificates \
    clang-format \
    flex \
    gtkwave \
    help2man \
    libfl-dev \
    python3 \
    python3-dev \
    python3-pip \
    python3-venv \
    universal-ctags \
    zlib1g-dev

# Clean apt cache and temporary files
RUN apt-get clean && \
    rm -rf /var/lib/apt/lists/*

#
# Build and install the tools
#

# Install Verilator (built from source)
# (Do this early as it takes a while).
RUN cd /tmp/verilator && \
    autoconf && \
    ./configure && \
    make -j$(nproc) && \
    make install && \
    cd /tmp && \
    rm -rf verilator

# Install Verible (download from chipsalliance)
RUN mkdir -p /opt/verible && \
    tar -x -f /tmp/${VERIBLE_ARCHIVE} -C /opt/verible --strip-components 1 && \
    rm -rf /tmp/${VERIBLE_ARCHIVE}
ENV PATH="/opt/verible/bin:${PATH}"

# Install uv (Python package manager) system-wide so any user can run it.
# (See: https://github.com/astral-sh/uv)
RUN curl -fsSL https://astral.sh/uv/install.sh \
    | env UV_INSTALL_DIR=/usr/local/bin UV_UNMANAGED_INSTALL=/usr/local/bin sh

# Install Copilot CLI
RUN curl -fsSL https://gh.io/copilot-install | bash
