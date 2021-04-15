FROM ubuntu:focal

RUN export DEBIAN_FRONTEND=noninteractive && \
    apt-get update && \
    apt-get install -y build-essential curl git libssl-dev ninja-build unzip wget zip && \
    # cmake
    cd /tmp && \
    wget https://github.com/Kitware/CMake/releases/download/v3.18.4/cmake-3.18.4.tar.gz && \
    tar zxvf cmake-3.18.4.tar.gz && \
    cd cmake-3.18.4 && \
    ./bootstrap && \
    make && \
    make install && \
    ln -s /usr/local/bin/cmake /usr/bin/ && \
    # vcpkg
    cd /opt && \
    git clone https://github.com/Microsoft/vcpkg.git && \
    cd vcpkg && \
    ./bootstrap-vcpkg.sh && \
    ./vcpkg install cxxopts && \
    # build
    cd /opt && \
    git clone https://github.com/EvanMcBroom/microsocks11.git && \
    cd microsocks11 && mkdir builds && cd builds && \
    cmake .. -DCMAKE_TOOLCHAIN_FILE=/opt/vcpkg/scripts/buildsystems/vcpkg.cmake && \
    cmake --build .

FROM ubuntu:focal

COPY --from=0 /opt/microsocks11/builds/microsocks /usr/bin

ENTRYPOINT ["/usr/bin/microsocks"]
