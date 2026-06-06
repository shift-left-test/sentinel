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
        graphviz \
        libgmock-dev \
        libgtest-dev \
        libncurses5-dev \
        libzstd-dev \
        locales \
        ninja-build \
        pkg-config \
        python3 \
        python3-pip \
        zlib1g-dev && \
    locale-gen en_US.UTF-8 && \
    rm -rf /var/lib/apt/lists/*

# Install LLVM 12 (available in the Ubuntu 20.04 default repositories)
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        libclang-12-dev \
        llvm-12-dev && \
    rm -rf /var/lib/apt/lists/*

RUN python3 -m pip install --no-cache-dir -U pip && \
    python3 -m pip install --no-cache-dir -U \
        cpplint \
        gcovr

ENV LANG=en_US.UTF-8
ENV TZ=Asia/Seoul
ENV GIT_SSL_NO_VERIFY=true

CMD ["/bin/bash"]
