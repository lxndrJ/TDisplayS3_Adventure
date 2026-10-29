#!/bin/bash

# Setup script for OpenCode integration in dev container

echo "Setting up OpenCode environment in dev container..."

# Check if we're inside Docker
if [ -f /.dockerenv ]; then
    echo "Running inside Docker container"
    
    # Ensure OpenCode is installed or accessible
    if ! command -v opencode &> /dev/null; then
        echo "OpenCode CLI not found. Installing..."
        curl -fsSL https://opencode.ai/install.sh | sh
    fi
    
    # Configure Ollama connection
    echo "Configuring Ollama connection to 10.99.50.110:11434"
    
    # Create OpenCode config directory if it doesn't exist
    mkdir -p /root/.config/opencode/
    
    # Set up basic OpenCode configuration using Ollama at the host IP
    cat > /root/.config/opencode/opencode.json << EOL
{
  "$schema": "https://opencode.ai/config.json",
  "providers": {
    "ollama": {
      "name": "Ollama (10.99.50.110)",
      "settings": {
        "baseURL": "http://10.99.50.110:11434/v1"
      }
    }
  }
}
EOL

    echo "OpenCode environment setup complete."
else
    echo "Not inside Docker container. Skipping container-specific setup."
fi

echo "Setup completed!"