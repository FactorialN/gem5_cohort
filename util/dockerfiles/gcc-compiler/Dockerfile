FROM  ghcr.io/gem5/ubuntu-24.04_all-dependencies:latest

LABEL org.opencontainers.image.source=https://github.com/gem5/gem5
LABEL org.opencontainers.image.licenses=BSD-3-Clause

ARG version=14 # Version of GCC to install in this image. Default is 14.

RUN apt -y update && \
    apt -y install gcc-${version} g++-${version} && \
    update-alternatives --install \
        /usr/bin/g++ g++ /usr/bin/g++-${version} 100 && \
    update-alternatives --install \
        /usr/bin/gcc gcc /usr/bin/gcc-${version} 100 && \
    update-alternatives --install \
        /usr/bin/c++ c++ /usr/bin/g++-${version} 100 && \
    update-alternatives --install \
        /usr/bin/cc cc /usr/bin/gcc-${version} 100
