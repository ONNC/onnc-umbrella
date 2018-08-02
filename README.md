# ONNC Umbrella

## Introduction

ONNC Umbrella is the umbrella for all ONNC projects.

## Prerequisites
  1. git
  2. Autotools
  3. GNU Libtool
  4. pkg-config
  5. Protocol Buffer

### Ubuntu - with Apt ###

```
apt install git automake libtool protobuf-compiler libprotoc-dev python2.7 python2.7-dev python-pip
pip install lit
```

### macOS - with Homebrew ###

```
xcode-select --install
brew install git autoconf automake libtool pkg-config protobuf
```

## Compilation Instructions
The simplest way to compile this package is:

Dance with `build.sh` script.

1. Check out ONNC from the repository.

    ```
    git clone --recursive https://github.com/ONNC/onnc-umbrella.git ${ONNC}
    ```

2. Type `cd ${ONNC}` to the directory containing the source code.

3. Use `build.sh` to compile the package.

    ```
    ./build.sh [mode] [target folder]
    ```

The package is built at `./build-<mode>/`, and installed
to `./install-<mode>/`. It is a staged installation if the
third argument (target folder) is given. Check GNU automake's DESTDIR for
more information about a staged installation at the below
[link](https://www.gnu.org/software/automake/manual/html_node/DESTDIR.html).

For example, use `./build.sh dbg /opt/onnc` to build the latest
revision in debug mode for installation at `/opt/onnc`.

It will build in `build-dbg`, install to `install-dbg`, and
generate a binary package named `onnc-<branch name>.tar.gz`.

## Deployment Instructions
You can use the following instruction to install the binary package.

```
tar zxf onnc-master.tar.gz -C /opt
```

or

```
mkdir /opt/onnc
tar zxf onnc-master.tar.gz --strip-components 1 -C /opt/onnc
```

Recompile with the same arguments `./build.sh dbg /opt/onnc`, or
make with `DESTDIR=./install-dbg` to ensure the binaries are installed
to the staged installation path instead of being installed directly to the
target folder `/opt/onnc`.

```
cd ./build-dbg
make DESTDIR=$(pwd)/../install-dbg install
```

### Building Mode

There are four building modes, regression mode, debugging mode, optimizing
mode and normal mode. Select mode by changing the second argument.

| mode    | description                                                  |
|---------|--------------------------------------------------------------|
| normal  | (default) build with normal compilation flags                |
| dbg     | build in debug mode (unittest enabled)                       |
| rgn     | build in debug mode (unittest enabled) with regression test  |
| opt     | build in optimized mode                                      |

### Target Folder

The package is configured to be installed at the target folder (PREFIX).

## Directory Structure

* README    - This document
* README.md - Same document in Markdown format
* build.sh  - The building script
* src       - The source directory of onnc project
* external  - The external librarys

## Compilation Instructions (CMake, experimental)
We also provide `build.cmake.sh` for people love cmake. Currently, this script
is experimental. It only pass deployment test on ubuntu Linux environment.

1. Check out ONNC from the repository with `--recursive` flag

    ```
    git clone --recursive https://github.com/ONNC/onnc-umbrella.git ${ONNC}
    ```

2. Type `cd ${ONNC}` to the directory containing the source code.

3. Use `build.cmake.sh` to configure the package.
    ```
    ./build.cmake.sh
    ```

The script creates `build` folder as a workspace.
