# Building

## Git Submodules

This project makes use of git submodules to import dependencies into the source tree.
After cloning this repository using git, prior to building NSTool the dependencies need to be downloaded.
Run these two commands to initialise and download the dependencies:

```bash
git submodule init
git submodule update
```

## Linux (including Windows Subsystem for Linux) & MacOS - Makefile

### Requirements

* `make`
* Terminal access
* Typical GNU compatible development tools (e.g. `clang`, `g++`, `c++`, `ar` etc) with __C++11__ support

### Using Makefile

* `make` (default) - Compile program
* Compiling the program requires local dependencies to be compiled via `make deps` beforehand
* `make clean` - Remove executable and object files
* `make deps` - Compile locally included dependency libraries
* `make clean_deps` - Remove compiled library binaries and object files

## Native Windows - Visual Studio

### Requirements

* [Visual Studio Community](https://visualstudio.microsoft.com/vs/community/) 2015 / 2017 / 2019 / 2022

### Compiling using the Visual Studio GUI

* Open `build/visualstudio/nstool.sln` in Visual Studio
* Select Target (e.g `Debug`|`Release` & `x86`|`x64`)
* Navigate to `Build`->`Build Solution`

### Compiling using the command line

* Open PowerShell
* Paste and run the following command after double-checking the paths.

```bash
& "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin\MSBuild.exe" `
  "{full path to the cloned repository}\nstool\build\visualstudio\nstool.sln" `
  /p:Platform=x64 `
  /p:Configuration=Release
```
