# Workflow
## Basic Workflow
#### If you are trying to fix build:
    Coming soon...
#### If you are trying to fix tests:
    Coming soon...
#### If you are wanting to do more:
    Coming soon...

# FAQ

## What CI does?
    $ ./build.sh all
## What these docker images are?
  1. `onnc-build`
    Coming soon...
  2. `onnc-prebuilt-external`
    Coming soon...
  3. `onnc-prebuilt`
    Coming soon...
## What these scripts are?
  1. `build.sh`
    Coming soon...
  2. `get-result.sh`
    Coming soon...
  4. `pull-image.sh`
    Coming soon...
  5. `push-image.sh`
    Coming soon...

# Workflow 2 with CMake
## start
```
$ docker pull iclink/onnc_dev:work
$ docker run -it --rm iclink/onnc_dev:work
```
## docker image management
  1. Dockerfile.dev
    + purpose
      + for minimal build, used by gitlab CI 
      + see onnc/.gitlab-ci.yml
    + build image and push to Docker Hub

      ```
      $ ./image-build.sh
      ```

  2. Dockerfile.dev.work
    + purpose
      + offer daily tools for work, clang-tidy, cgdb, etc.
      + depend on Dockerfile.dev
    + build image and push to Docker Hub

      ```
      $ ./image-build-work.sh
      ```
