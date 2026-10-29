#!/bin/bash

# Writes the OpenCode config for the current user (remoteUser "opencode").
# Called at container runtime via postCreateCommand, not at image build time.
# Ollama endpoint comes from OLLAMA_HOST, set in .devcontainer/devcontainer.env
# (gitignored; copy from devcontainer.env.example).

set -euo pipefail

# devcontainer.env is only applied to the devcontainer CLI process, not
# injected into the container, so lifecycle commands never see it. Load it
# ourselves from the bind-mounted workspace. Values already present in the
# environment win.
ENV_FILE="$(cd "$(dirname "$0")" && pwd)/devcontainer.env"
if [ -f "${ENV_FILE}" ]; then
    while IFS='=' read -r key value; do
        case "${key}" in ''|\#*) continue ;; esac
        if [ -z "${!key:-}" ]; then
            export "${key}=${value}"
        fi
    done < "${ENV_FILE}"
fi

if [ -z "${OLLAMA_HOST:-}" ]; then
    echo "ERROR: OLLAMA_HOST is not set." >&2
    echo "Copy .devcontainer/devcontainer.env.example to devcontainer.env and set it there." >&2
    exit 1
fi

mkdir -p "${HOME}/.config/opencode"

cat > "${HOME}/.config/opencode/opencode.json" << EOL
{
  "\$schema": "https://opencode.ai/config.json",
  "providers": {
    "ollama": {
      "name": "Ollama (${OLLAMA_HOST})",
      "settings": {
        "baseURL": "${OLLAMA_HOST}/v1"
      }
    }
  }
}
EOL

echo "OpenCode configured for Ollama at ${OLLAMA_HOST}."
