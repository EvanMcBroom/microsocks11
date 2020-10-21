FROM ubuntu:focal

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y build-essential curl git libssl-dev ninja-build unzip wget && \
    # cmake \
    cd /tmp && \
    wget https://github.com/Kitware/CMake/releases/download/v3.18.4/cmake-3.18.4.tar.gz && \
    tar zxvf cmake-3.18.4.tar.gz && \
    cd cmake-3.18.4 && \
    ./bootstrap && \
    make && \
    make install && \
    ln -s /usr/local/bin/cmake /usr/bin/

RUN ls && \
    # vcpkg \
    cd /opt && \
    git clone https://github.com/Microsoft/vcpkg.git && \
    cd vcpkg && \
    ./bootstrap-vcpkg.sh && \
    ./vcpkg install cxxopts

RUN ls && \
    # build \
    cd /opt && \
    git clone https://github.com/EvanMcBroom/microsocks11.git && \
    cd microsocks11 && mkdir builds && cd builds && \
    cmake .. -DCMAKE_TOOLCHAIN_FILE=/opt/vcpkg/scripts/buildsystems/vcpkg.cmake && \
    cmake --build . && \
    ln -s /opt/microsocks11/builds/microsocks /usr/bin

ENTRYPOINT /usr/bin/microsocks
