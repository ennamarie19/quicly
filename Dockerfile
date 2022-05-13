# Build Stage
FROM --platform=linux/amd64 ubuntu:20.04 as builder

## Install build dependencies.
RUN apt-get update && \
    DEBIAN_FRONTEND=noninteractive apt-get install -y cmake clang libssl-dev git build-essential

## Add source code to the build stage.
RUN git clone --recursive https://github.com/ennamarie19/quicly.git
WORKDIR /quicly
RUN git checkout mayhem

## TODO: ADD YOUR BUILD INSTRUCTIONS HERE.
RUN mkdir -p build
WORKDIR build
RUN CC=clang CXX=clang++ cmake -D BUILD_FUZZER=1 ..
RUN make

# Package Stage
FROM --platform=linux/amd64 ubuntu:20.04

## TODO: Change <Path in Builder Stage>
COPY --from=builder /quicly/build/quicly-fuzzer-packet /
COPY --from=builder /quicly/deps /deps
COPY --from=builder /lib /lib
