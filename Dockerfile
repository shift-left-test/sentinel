# Copyright (c) 2026 LG Electronics Inc.
# SPDX-License-Identifier: MIT

FROM ubuntu:20.04

ARG DEBIAN_FRONTEND=noninteractive

RUN echo "dash dash/sh boolean false" | debconf-set-selections && \
    dpkg-reconfigure dash

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        build-essential \
        ca-certificates \
        cmake \
        cppcheck \
        doxygen \
        file \
        g++ \
        gcc \
        git-core \
        gnupg \
        gpg-agent \
        graphviz \
        libncurses5-dev \
        libzstd-dev \
        zlib1g-dev \
        locales \
        ninja-build \
        pkg-config \
        python3 \
        python3-pip \
        software-properties-common \
        wget && \
    locale-gen en_US.UTF-8 && \
    rm -rf /var/lib/apt/lists/*

# Install LLVM 14
RUN echo "deb http://apt.llvm.org/focal/ llvm-toolchain-focal-14 main" \
        > /etc/apt/sources.list.d/llvm.list && \
    wget -qO - https://apt.llvm.org/llvm-snapshot.gpg.key | apt-key add - && \
    apt-get update && \
    apt-get install -y --no-install-recommends \
        libclang-14-dev \
        llvm-14-dev && \
    apt-get download libpolly-14-dev && \
    dpkg --force-all -i libpolly-14-dev*.deb && \
    rm -f libpolly-14-dev*.deb && \
    rm -rf /var/lib/apt/lists/*

RUN python3 -m pip install --no-cache-dir -U pip && \
    python3 -m pip install --no-cache-dir -U \
        cpplint \
        gcovr

# GoogleTest (1.15 for ThrowsMessage support, requires 1.12+)
RUN wget -qO /tmp/googletest.tar.gz \
        https://github.com/google/googletest/releases/download/v1.15.2/googletest-1.15.2.tar.gz && \
    tar -xzf /tmp/googletest.tar.gz -C /tmp && \
    cmake -S /tmp/googletest-1.15.2 -B /tmp/googletest-build && \
    cmake --build /tmp/googletest-build --target install && \
    rm -rf /tmp/googletest*

ENV LANG=en_US.UTF-8
ENV TZ=Asia/Seoul
ENV GIT_SSL_NO_VERIFY=true

CMD ["/bin/bash"]
