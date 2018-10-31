# Using docker

Compiled onnc ([Open Neural Network Compiler](https://onnc.ai)) and execution environments are packed into docker images for a quick view.

# Get docker image

## Normal mode

Pull docker image from Dockerhub with the command below.

Normal mode image contains onnc-umbrella and essential dependencies built with common configurations, specialized for general usage without building onnc.

```shell
$ docker pull onnc/onnc
```

## Debug mode

Dedug mode image was built with debug informations, focus on general developerd that may rebuilding onnc.

```shell
$ docker pull onnc/onnc-debug
```

# Run test

## Driven by ctest

All unit tests can be driven by CTest, the CMake test driver program.

```shell
$ docker run -ti --rm onnc/onnc-debug ctest
```

## Run each unit test

Each unit test can execute separately inside docker.

```shell
$ docker run -ti --rm onnc/onnc-debug bash
$ ./tools/unittests/unittest_<test_name>
```

Replace `<test_name>` with unit test name.

To leave container, simply use `exit`.

```shell
$ exit
```

Available unit tests:

* Any
* FileHandle
* ONNXReader
* StringSwitch
* BinaryTree
* JsonObject
* PassManager
* TensorSel
* JsonValue
* Quadruple
* ComputeGraph
* LivenessAnalysis
* ComputeIR
* StringMap
* Digraph
* MemAlloc
* StringRef

# Run onnx model

1. Prepare onnx model

You can use your custom model in onnx ([Open Neural Network Exchange](https://onnx.ai/)) format, with input data in protobuf ([Protocol Buffers](https://developers.google.com/protocol-buffers/)) format.

If you don't have one, you can download several onnx official models with test data from [model zoo](https://github.com/onnx/models).

2. Run commands

```
$ cd <model_directory>
$ docker run -ti --rm -v $PWD:/onnc/$PWD -w /onnc/$PWD onnc/onnc onni <model_file> <input_file>
```

Replace these arguments in your commands:

* `<model_directory>` : Directory that contains onnx model
* `<model_file>` : onnx model to execute
* `<input_file>` : Input file path

## Verbose level

Toggle verbose level with `--verbose=<level>` flag.

Example:

```
$ cd <model_directory>
$ docker run -ti --rm -v $PWD:/onnc/$PWD -w /onnc/$PWD onnc/onnc onni --verbose=3 <model_file> <input_file>
```

Informations of each verbose level:

* Level 1: Inference time & memory usage
* Level 2: Dump operators
* Level 3: Per layer inference time & operator counts
* Level 4: Allocated memory layout

Higher level will also contain informations of all lower levels.

# Build your docker image

You may modify onnc sources and need to build your onnc images. These are steps to rebuild them.

### Step 1: Modify source code in `src` directory


### Step 2: Run build script for docker

```shell
$ cd onnc-umbrella
$ ./docker/build.sh
```

It will build 2 major images:

* `onnc/onnc`: Normal mode build.
* `onnc/onnc-debug`: Debug mode build.

Other internal images may be built to accelerate building time.
