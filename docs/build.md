# Build & Test from onnc-umbrella

You can build onnc by onnc-umbrella as a simpler way. 

## Build

### Step 1: Choose building script
You can choose either CMake or Automake flavored building scripts. The usages of these scripts are the same, just driven by different building system.

+ `build.sh` : Driven with CMake
+ `build.cmake.sh` : Driven with Automake, it will **NOT** build Sophon target.

### Step 2: Run building script

There are 2 modes of building:

* `normal` : Normal build. *Recommanded in common usage*
* `dbg` : Debug build. *Recommanded for developer*

According to your choice, you can run either

```shell
$ ./build.sh MODE PREFIX
```

or

```shell
$ ./build.cmake.sh MODE PREFIX
```

Remenber to replace `MODE` with your building mode (e.g: `normal` or `dbg`).

It will configure, build onnc and generate an installation package.

`PREFIX` means the installation prefix. You can drop it to install into the default path, `/opt/onnc`, or replace with your choice.

### Environment variables

|Name|Available values|Default|Description|
|----|----------------|-------|-----------|
|MAX_MAKE_JOBS| Number | | Max parallels of `make` jobs (e.g. `make -j`)|

## Install

After building, you'll get an `onnc-<branch_name>.tar.gz` tarball, the `<branch_name>` will be replaced by the git branch name of onnc-umbrella.

Then you can run:

```shell
$ tar -zxf onnc-<branch_name>.tar.gz
```

It will unzip this tarball and install to `PREFIX`.

## Test

All the unittests reside in `build-MODE/tools/unittests`. You can run each unittest to test related parts. 