# Use Microsoft devcontainers base image (Ubuntu LTS)
FROM mcr.microsoft.com/vscode/devcontainers/base:0-ubuntu-22.04

# Avoid interactive prompts
ARG DEBIAN_FRONTEND=noninteractive

# Install build tools (native linux build) and mingw-w64 for cross-compiling Windows builds
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    git \
    make \
    cmake \
    doxygen \
    pkg-config \
    ca-certificates \
    curl \
    mingw-w64 \
    gcc \
    g++ \
    clang-format \
    && apt-get clean && rm -rf /var/lib/apt/lists/*

# Create workspace directory
RUN mkdir -p /workspaces/nstool
WORKDIR /workspaces/nstool

# Use bash as default shell
SHELL ["/bin/bash", "-lc"]
