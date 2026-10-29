FROM alpine:3

# Base tools. gcompat + libstdc++ are required so the glibc-built ESP32
# toolchains that PlatformIO downloads (xtensa/riscv gcc) run on musl.
RUN apk add --no-cache \
    git \
    curl \
    bash \
    python3 \
    py3-pip \
    nodejs \
    npm \
    libstdc++ \
    gcompat

# PlatformIO Core in a venv (Alpine's Python is PEP 668 externally managed);
# "pio" must be on PATH for the postCreateCommand
# (pio pkg install -e t_display_s3).
RUN python3 -m venv /opt/platformio \
    && /opt/platformio/bin/pip install --no-cache-dir platformio \
    && ln -s /opt/platformio/bin/pio /usr/local/bin/pio \
    && ln -s /opt/platformio/bin/piodebuggdb /usr/local/bin/piodebuggdb \
    && ln -s /opt/platformio/bin/piod /usr/local/bin/piod

# OpenCode CLI, installed globally via npm so it is available to every user
# (remoteUser is "opencode", not root)
RUN npm install -g opencode-ai

# Non-root user expected by devcontainer.json "remoteUser".
# The common-utils feature installs zsh and configures it as default shell.
RUN adduser -D -s /bin/bash opencode

# The workspace is bind-mounted at runtime by the devcontainer runtime;
# no project files are baked into the image.
WORKDIR /workspaces/TDisplayS3_Adventure
