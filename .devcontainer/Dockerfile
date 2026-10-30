FROM debian:bookworm-slim

# glibc base: the VSCodium server binary is glibc-built and fails on
# Alpine/musl even with gcompat ("fcntl64: symbol not found"). The
# ESP32 toolchains PlatformIO downloads (xtensa/riscv gcc) are also
# glibc-built, so they run natively here as well.
RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        git \
        curl \
        bash \
        python3 \
        python3-venv \
        python3-pip \
        nodejs \
        npm \
        ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# PlatformIO Core in a venv (Debian's Python is PEP 668 externally managed);
# "pio" must be on PATH for the postCreateCommand
# (pio pkg install -e t_display_s3).
RUN python3 -m venv /opt/platformio \
    && /opt/platformio/bin/pip install --no-cache-dir platformio \
    && ln -s /opt/platformio/bin/pio /usr/local/bin/pio \
    && ln -s /opt/platformio/bin/piodebuggdb /usr/local/bin/piodebuggdb \
    && ln -s /opt/platformio/bin/piod /usr/local/bin/piod

# OpenCode CLI, installed globally via npm so it is available to every user
# (remoteUser is "opencode", not root)
RUN npm install -g @opencode/cli

# Non-root user expected by devcontainer.json "remoteUser".
# The common-utils feature installs zsh and configures it as default shell.
RUN useradd -m -s /bin/bash opencode

# The workspace is bind-mounted at runtime by the devcontainer runtime;
# no project files are baked into the image.
WORKDIR /workspaces/TDisplayS3_Adventure
