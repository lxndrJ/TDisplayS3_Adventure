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

# Docker-in-Docker: the official devcontainer feature only supports
# Debian/Ubuntu, not Alpine, so install the Alpine packages instead and start
# dockerd via the entrypoint script below. Requires "runArgs": ["--privileged"]
# in devcontainer.json.
RUN apk add --no-cache \
    docker \
    docker-cli-compose

# The dev container itself runs on an overlay filesystem, so the nested
# daemon cannot mount overlay2-on-overlay2 ("invalid argument"). The vfs
# storage driver works everywhere but copies layers instead of hardlinking,
# so it is slower and uses more disk.
RUN mkdir -p /etc/docker \
    && printf '{"storage-driver":"vfs"}\n' > /etc/docker/daemon.json

# Non-root user expected by devcontainer.json "remoteUser".
# The common-utils feature installs zsh and configures it as default shell.
# Membership in the "docker" group (created by the docker package) lets the
# user talk to the daemon without sudo.
RUN adduser -D -s /bin/bash opencode \
    && addgroup opencode docker

# Entrypoint that starts dockerd in the background before handing off to the
# container's normal command (mirrors the docker-in-docker feature).
COPY docker-init.sh /usr/local/share/docker-init.sh
RUN chmod +x /usr/local/share/docker-init.sh
ENTRYPOINT ["/usr/local/share/docker-init.sh"]

# The workspace is bind-mounted at runtime by the devcontainer runtime;
# no project files are baked into the image.
WORKDIR /workspaces/TDisplayS3_Adventure
