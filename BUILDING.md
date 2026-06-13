# Building

## Before you begin

Before attempting to build make sure you've cloned the repository and pulled the dependencies.

`git clone --recursive https://github.com/jakcron/nstool.git`

or

```
git clone https://github.com/jakcron/nstool.git
cd nstool
git submodule init
git submodule update
```

There are multiple ways to build this application.

* Using VSCode build tasks in a devcontainer (recommended)
* `cmake` on Linux, macOS, or Windows Subsystem for Linux
* `make` on Linux, macOS, or Windows Subsystem for Linux
* Visual Studio Community solution

## Build target summary

The various ways of building this application for a specific operating system depend on the host operating system (OS) and strategy you use. You can build for macOS only if your host OS is macOS.

### devcontainer / cmake

| 🔽 Host / ▶️ Target         | ![Linux][linux-image] | ![macOS][macos-image] | ![Windows][windows-image] |
| ------------------------- | --------------------- | --------------------- | ------------------------- |
| ![Linux][linux-image]     | ✅                     |                       | ✅                         |
| ![macOS][macos-image]     | ✅                     | ✅                     | ✅                         |
| ![Windows][windows-image] | ✅                     |                       | ✅                         |

### make

| 🔽 Host / ▶️ Target         | ![Linux][linux-image] | ![macOS][macos-image] | ![Windows][windows-image] |
| ------------------------- | --------------------- | --------------------- | ------------------------- |
| ![Linux][linux-image]     | ✅                     |                       |                           |
| ![macOS][macos-image]     | ✅                     | ✅                     | ✅                         |
| ![Windows][windows-image] | ✅                     |                       | ✅                         |

### Visual Studio Community

| 🔽 Host / ▶️ Target         | ![Linux][linux-image] | ![macOS][macos-image] | ![Windows][windows-image] |
| ------------------------- | --------------------- | --------------------- | ------------------------- |
| ![Linux][linux-image]     |                       |                       |                           |
| ![macOS][macos-image]     |                       | ✅                     |                           |
| ![Windows][windows-image] |                       |                       | ✅                         |

## Build tasks in a devcontainer (recommended)

Dev containers are recommended because every developer is guaranteed to be using the same development environment. This removes the chance of a build task succeeding for one person, but failing for someone else.

These are initial steps. Once you have the Dev Container extension VSCode will automatically detect the setup and prompt you to reopen VSCode inside the devcontainer.

* Make sure you have a container manager installed. This could be Docker Desktop (Linux, macOS, and Windows) or podman (Linux).
* Open the repository with [VSCode][vscode]
* Install the Dev Container extension (`ms-vscode-remote.remote-containers`)
* Run the `Dev Containers: Open Folder in Container` command

Now you can use the VSCode built in tasks feature to build the application.

### Terminal > Run Task...

Cmake is fast and smart. The dependencies are automatically built and monitored for changes.

The build tasks below include reconfiguring so you don't need to manually tell cmake to reconfigure if you've edited a header file.

| Task                   | Explanation                                                                                                               |
| ---------------------- | ------------------------------------------------------------------------------------------------------------------------- |
| `Build All Platforms`  | ![Linux][linux-image] ![Windows][windows-image] Builds for Linux and Windows.                                             |
| `Build nstool (Linux)` | ![Linux][linux-image] Builds for Linux.                                                                                   |
| `Build nstool (macOS)` | ![macOS][macos-image] Builds for macOS. Requires that your host OS is macOS.                                              |
| `Clean Build`          | Removes the /build/cmake directory to clear cmake caches.                                                                 |
| `Clean Binaries`       | Removes built binaries.                                                                                                   |
| `Clean All`            | Runs both clean tasks.                                                                                                    |
| `Build Solution`       | ![Windows][windows-image] Builds for Windows using a PowerShell script. Requires Visual Studio Community to be installed. |
| `Clean Solution`       | Clears the solution cache.                                                                                                |

## `cmake` on Linux, macOS, or Windows

Refer to [BUILDING_CMAKE](BUILDING_CMAKE.md)` for instructions.

## `make` on Linux, macOS, or Windows Subsystem for Linux

### Requirements

* `make`
* Terminal access
* GNU compatible development tools (e.g. `clang`, `g++`, `c++`, `ar`) with __C++11__ support

### Steps

* Run `make deps` to compile the dependencies.
* Run `make PROJECT_PLATFORM=GNU program` to compile the program.

## Visual Studio Community solution

A solution file is provided in `/build/visualstudio/nstool.sln`.

### Requirements
* [Visual Studio Community][visual-studio] 2015 / 2017 / 2019

### Steps - Using VSCode

* Run `Terminal` > `Run Task...` > `Build Solution`

### Steps - Using Visual Studio Community

* Open `build/visualstudio/nstool.sln` in Visual Studio
* Select Target (e.g `Debug`|`Release` & `x86`|`x64`)
* Navigate to `Build` > `Build Solution`

[vscode]: https://code.visualstudio.com/
[visual-studio]: https://visualstudio.microsoft.com/vs/community/
[linux-image]: https://img.shields.io/badge/Linux-FCC624?logo=linux&logoColor=black
[macos-image]: https://img.shields.io/badge/macOS-000000?logo=apple&logoColor=white
[windows-image]: https://img.shields.io/badge/Windows-0078D6?logo=windows&logoColor=white