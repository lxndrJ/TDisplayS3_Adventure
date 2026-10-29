FROM ghcr.io/lxndrj/tdisplay-s3-adventure:latest

# Additional tools and dependencies needed for development
RUN apk add --no-cache \
    git \
    curl \
    bash \
    python3 \
    py3-pip \
    nodejs \
    npm

# OpenCode CLI, installed globally via npm so it is available to every user
# (remoteUser is "opencode", not root)
RUN npm install -g opencode-ai

# The workspace is bind-mounted at runtime by the devcontainer runtime;
# no project files are baked into the image.
WORKDIR /workspaces/TDisplayS3_Adventure
