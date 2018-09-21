FROM ubuntu:18.04

RUN groupadd --gid 1000 onnc \
    && useradd --uid 1000 --gid onnc --shell /bin/bash --create-home onnc \
    && mkdir -p /etc/sudoers.d \
    && echo 'onnc ALL=(ALL:ALL) NOPASSWD:ALL' >> /etc/sudoers.d/onnc \
    && chmod 440 /etc/sudoers.d/onnc

RUN sed -i 's/archive.ubuntu.com/debian.linux.org.tw/' /etc/apt/sources.list \
    && apt-get update \
    && apt-get install -y --no-install-recommends \
        curl \
        sudo \
        gcc g++ git cmake automake libtool protobuf-compiler libprotoc-dev pkg-config python2.7 python2.7-dev python-pip libgoogle-glog-dev libboost-filesystem-dev flex bison curl \
        libllvm5.0 llvm-5.0 llvm-5.0-dev llvm-5.0-doc llvm-5.0-examples llvm-5.0-runtime llvm-5.0-tools \
    && rm -rf /var/lib/apt/lists/*

USER onnc

RUN pip install setuptools

COPY --chown=onnc:onnc ./external/onnx /home/onnc/onnx
RUN pip install /home/onnc/onnx

WORKDIR /home/onnc/onnc-umbrella
RUN sudo chown onnc:onnc /home/onnc/onnc-umbrella

COPY --chown=onnc:onnc ./external /home/onnc/onnc-umbrella/external/
COPY --chown=onnc:onnc ./scripts /home/onnc/onnc-umbrella/scripts/
COPY --chown=onnc:onnc ./build.* /home/onnc/onnc-umbrella/

RUN mkdir -p /home/onnc/onnc-umbrella/src
RUN sed -i 's/^  build_llvm/  #build_llvm/' scripts/library.sh \
    && sed -i 's/LLVM_ROOT_DIR="\${ONNC_EXTDIR}"/LLVM_ROOT_DIR="\/lib"/' build.cmake.sh
RUN MAX_MAKE_JOBS=8 \
    EXTERNAL_ONLY=true \
    ./build.cmake.sh dbg install-onnc


COPY --chown=onnc:onnc ./ /home/onnc/onnc-umbrella/
RUN MAX_MAKE_JOBS=8 \
    BUILD_EXTERNAL=false \
    ./build.cmake.sh dbg
