# Using docker

We also provide docker images for a quick view. You can follow these steps:

# Get docker image

## Normal mode

You can pull docker image from dockerhub with this command.

It contains onnc-umbrella and essential dependencies built with common configurations.

```shell
$ docker pull onnc/onnc
```

## Debug mode

There's also a dedug mode image. The onnc was built with debug configurations.

```shell
$ docker pull onnc/onnc-debug
```

# Run test

## Driven by ctest

If you want to see overall results, you can drive all unit tests by ctest.

```shell
$ docker run -ti --rm onnc/onnc-debug ctest
```

## Run each unit test

You may like to run each unit test inside docker separately.

```shell
$ docker run -ti --rm onnc/onnc-debug bash
$ ./tools/unittests/unittest_<test_name>
```

Replace `<test_name>` with the unit test name you'd like to run.

After finish your jobs, you can simply use `exit` to leave container.

```shell
$ exit
```

# Run onnx model

Prepare your onnx module and protobuf input file, and run these commands.

```
$ cd <model_directory>
$ docker run -ti --rm -v $PWD:/onnc/$PWD -w /onnc/$PWD onnc/onnc onni <model_file> <input_file>
```

Replace these arguments in your commands:

* `<model_directory>` : Directory that contains onnx model
* `<model_file>` : onnx model to execute
* `<input_file>` : Input file path

# Build your docker image

You may modify onnc sources and need to build your onnc images. These are steps to rebuild them.

### Step 1: Get onnc-umbrella from GitHub

```shell
$ git clone --recursive https://github.com/ONNC/onnc-umbrella.git
```

### Step 2: Run build script for docker

```shell
$ cd onnc-umbrella
$ ./docker/build.sh
```

It will build 3 images:

* `onnc/onnc:external-prebuilt`: Pre-build external dependencies to accelerate building time.
* `onnc/onnc:normal`: Normal mode build.
* `onnc/onnc-dev`: Debug mode build.
