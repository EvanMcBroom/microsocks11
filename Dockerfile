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
    cmake --build . && \
    mv /opt/microsocks11/builds/microsocks /usr/bin && \
    # cleanup
    cd /opt && \
    rm -rf microsocks11 && \
    rm -rf vcpkg && \
    cd /tmp/cmake-3.18.4 && \
    make uninstall && \
    cd /tmp && \
    rm -rf cmake* && \
    apt-get remove --purge -y build-essential curl git libssl-dev ninja-build unzip wget zip && \
    apt-get autoremove -y && \
    apt-get clean -y

ENTRYPOINT ["/usr/bin/microsocks"]
