FROM ghcr.io/lxndrj/tdisplay-s3-adventure:latest

# Install additional tools and dependencies needed for development
RUN apk add --no-cache \
    git \
    curl \
    bash \
    python3 \
    py3-pip \
    nodejs \
    npm \
    && pip3 install --upgrade pip

# Set working directory
WORKDIR /workspace/TDisplayS3_Adventure

# Copy project files into the container
COPY . .

# Setup environment
COPY .devcontainer/setup-env.sh .
RUN chmod +x setup-env.sh

# Run setup script
RUN ./setup-env.sh

EXPOSE 11434

CMD ["tail", "-f", "/dev/null"]