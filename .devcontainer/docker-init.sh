#!/bin/sh
# Starts dockerd in the background on container start, then hands off to the
# normal container command. Alpine replacement for the official
# docker-in-docker devcontainer feature, which does not support Alpine.
# The container must run privileged ("runArgs": ["--privileged"] in
# devcontainer.json).

_log() { echo "[docker-init] $*"; }

if ! docker info >/dev/null 2>&1; then
    _log "Starting dockerd..."
    if [ "$(id -u)" = "0" ]; then
        nohup dockerd >/tmp/dockerd.log 2>&1 &
    elif command -v sudo >/dev/null 2>&1 && sudo -n true >/dev/null 2>&1; then
        sudo sh -c 'nohup dockerd >/tmp/dockerd.log 2>&1 &'
    else
        _log "WARNING: not root and no passwordless sudo available; dockerd not started." >&2
    fi
else
    _log "Docker daemon already running."
fi

exec "$@"
