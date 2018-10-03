FROM onnc-dev-external-prebuilt

COPY --chown=onnc:onnc ./ /onnc/onnc-umbrella/

ARG MODE=normal

RUN MAX_MAKE_JOBS=${THREAD} \
    BUILD_EXTERNAL=false \
    ./build.cmake.sh ${MODE}

ENV PATH="/onnc/onnc-umbrella/install-${MODE}/bin:${PATH}"
WORKDIR /onnc/onnc-umbrella/build-${MODE}
