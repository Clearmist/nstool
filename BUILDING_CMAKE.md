# Building with CMake

This project supports building with CMake in addition to the legacy Makefile system.

## Prerequisites

### All Platforms
- CMake 3.12 or higher
- A C++11 compatible compiler
- A C11 compatible compiler

### Linux
- GCC or Clang
- Standard build tools (make, etc.)

### Windows (Cross-compilation from Linux)
- MinGW-w64 cross-compiler (`x86_64-w64-mingw32-gcc`, `x86_64-w64-mingw32-g++`)

### Windows (Native)
- Visual Studio 2015 or later, OR
- MinGW-w64

### macOS
- Xcode Command Line Tools

## Building for Linux

```bash
# Create a build directory
mkdir -p build/cmake
cd build/cmake

# Configure
cmake ../.. -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build . -j$(nproc)

# The executable will be in: bin/linux/nstool
```

## Building for Windows (Cross-compilation from Linux)

```bash
# Create a build directory
mkdir -p build/cmake
cd build/cmake

# Configure with MinGW toolchain
cmake ../.. -DCMAKE_TOOLCHAIN_FILE=mingw-w64-x86_64.cmake -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build . -j$(nproc)

# The executable will be in: bin/win32/nstool.exe
```

## Building for Windows (Native with MinGW)

```bash
# Create a build directory
mkdir -p build/cmake
cd build/cmake

# Configure
cmake ../.. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build . -j%NUMBER_OF_PROCESSORS%

# The executable will be in: bin/win32/nstool.exe
```

## Building for Windows (Native with Visual Studio)

```bash
# Create a build directory
mkdir -p build/cmake
cd build/cmake

# Configure (adjust Visual Studio version as needed)
cmake ../.. -G "Visual Studio 16 2019" -A x64

# Build
cmake --build . --config Release

# The executable will be in: bin/win32/nstool.exe
```

## Building for macOS

```bash
# Create a build directory
mkdir -p build/cmake
cd build/cmake

# Configure
cmake ../.. -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build . -j$(sysctl -n hw.ncpu)

# The executable will be in: bin/macos/nstool
```

## Build Types

CMake supports different build types:
- `Debug` - No optimization, with debug symbols
- `Release` - Optimized, no debug symbols
- `RelWithDebInfo` - Optimized, with debug symbols
- `MinSizeRel` - Optimized for size

Specify the build type during configuration:
```bash
cmake .. -DCMAKE_BUILD_TYPE=Release
```

## Clean Build

To clean and rebuild:
```bash
# Remove the build directory
rm -rf build/cmake

# Recreate and build
mkdir -p build/cmake
cd build/cmake
cmake ../.. -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

To clean binaries and dependencies:
```bash
# Remove binaries
rm -rf bin/

# Remove dependency binaries
rm -rf deps/*/bin/
```

## Output Directory Structure

The build system places outputs in platform-specific directories at the project root:
- Linux: `bin/linux/nstool`
- Windows: `bin/win32/nstool.exe`
- macOS: `bin/macos/nstool`

CMake build files are in: `build/cmake/`

Dependencies are built as static libraries in their respective directories:
- `deps/libfmt/bin/<platform>/libfmt.a`
- `deps/liblz4/bin/<platform>/liblz4.a`
- `deps/libmbedtls/bin/<platform>/libmbedtls.a`
- `deps/libtoolchain/bin/<platform>/libtoolchain.a`
- `deps/libpietendo/bin/<platform>/libpietendo.a`

## Troubleshooting

### MinGW Cross-Compilation Issues
If you encounter issues with MinGW cross-compilation, ensure the toolchain is installed:
```bash
sudo apt-get install mingw-w64
```

### CMake Version
Check your CMake version:
```bash
cmake --version
```

If it's below 3.12, update CMake or use the legacy Makefile build system.

### Parallel Builds
For faster builds, use parallel compilation:
- Linux/macOS: `-j$(nproc)` or `-j$(sysctl -n hw.ncpu)`
- Windows: `-j%NUMBER_OF_PROCESSORS%`

## Legacy Makefile Build System

The original Makefile build system is still available. See [BUILDING.md](BUILDING.md) for details.
