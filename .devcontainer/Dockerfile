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
    && apt-get clean && rm -rf /var/lib/apt/lists/*

# Create workspace directory
RUN mkdir -p /workspaces/nstool
WORKDIR /workspaces/nstool

# Copy helper scripts into the image and make them executable.
# During container creation these will be overwritten by the mounted workspace contents,
# but we provide them to ensure the container image has them available.
COPY .devcontainer/scripts /tmp/devcontainer-scripts
RUN chmod +x /tmp/devcontainer-scripts/*.sh && mkdir -p .devcontainer && mv /tmp/devcontainer-scripts .devcontainer/scripts

# Use bash as default shell
SHELL ["/bin/bash", "-lc"]

# Keep the image small and simple. Additional packages can be added as needed.