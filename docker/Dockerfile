FROM onnc/onnc-dev-external-prebuilt

COPY --chown=onnc:onnc ./ ./

ARG THREAD=8
ARG MODE=normal

RUN MAX_MAKE_JOBS=${THREAD} \
    BUILD_EXTERNAL=false \
    ./build.cmake.sh ${MODE}

RUN sudo mv ./docker/in-container/* /usr/bin/

ENV PATH="/onnc/onnc-umbrella/install-${MODE}/bin:${PATH}"
WORKDIR "/onnc/onnc-umbrella/build-${MODE}"
