# Copyright (c) 2026 LG Electronics Inc.
# SPDX-License-Identifier: MIT

FROM ubuntu:20.04 AS builder

ARG DEBIAN_FRONTEND=noninteractive

RUN echo "dash dash/sh boolean false" | debconf-set-selections
RUN dpkg-reconfigure dash

RUN apt-get update
RUN apt-get install -y gpg-agent locales software-properties-common
RUN locale-gen en_US.UTF-8

RUN add-apt-repository -y ppa:ubuntu-toolchain-r/test
RUN apt-get update
RUN apt-get install -y \
    build-essential \
    ca-certificates \
    cmake \
    cppcheck \
    doxygen \
    g++ \
    gcc \
    git-core \
    gnupg \
    graphviz \
    libgtest-dev \
    libncurses5-dev \
    libzstd-dev \
    ninja-build \
    pkg-config \
    python3 \
    python3-pip \
    wget

RUN python3 -m pip install -U pip
RUN python3 -m pip install -U \
    cpplint \
    gcovr

# Install LLVM 14
RUN echo "deb http://apt.llvm.org/focal/ llvm-toolchain-focal-14 main" > /etc/apt/sources.list.d/llvm.list
RUN wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key | apt-key add -
RUN apt-get update
RUN apt-get install -y \
    libclang-14-dev \
    llvm-14-dev
RUN apt download libpolly-14-dev && dpkg --force-all -i libpolly-14-dev* && rm -rf libpolly-14-dev*.deb

# GoogleTest
RUN cmake -S /usr/src/googletest -B /tmp/googletest && \
    cmake --build /tmp/googletest --target install

RUN apt-get clean
RUN rm -rf /var/lib/apt/lists/*

FROM ubuntu:20.04

COPY --from=builder /usr /usr
COPY --from=builder /etc /etc
COPY --from=builder /var /var

ENV LANG=en_US.UTF-8
ENV TZ=Asia/Seoul
ENV GIT_SSL_NO_VERIFY=true

CMD ["/bin/bash"]
