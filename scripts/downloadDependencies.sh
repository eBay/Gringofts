#!/bin/bash
# This script downloads all external dependencies

mkdir ~/temp

# preparation
apt-get update -y &&
  apt-get install -y wget tar git build-essential
# download cmake 3.12
cd ~/temp && version=3.12 && build=0 &&
  wget https://cmake.org/files/v$version/cmake-$version.$build.tar.gz &&
  tar -xzvf cmake-$version.$build.tar.gz &&
  mv cmake-$version.$build cmake
# download and install clang/clang++ 6.0.1
wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key | apt-key add - &&
  apt-get install -y software-properties-common &&
  apt-add-repository "deb http://apt.llvm.org/xenial/ llvm-toolchain-xenial-6.0 main" &&
  apt-get update -y &&
  apt-get install -y clang-6.0
# download and install gcc/g++ 7.3.0
apt-get install -y software-properties-common &&
  add-apt-repository ppa:ubuntu-toolchain-r/test -y &&
  apt-get update -y &&
  apt-get install -y gcc-7 g++-7 -y
# download prometheus cpp client
apt-get install libcurl4-gnutls-dev -y &&
  cd ~/temp && version=v0.4.2 &&
  git clone -b $version https://github.com/jupp0r/prometheus-cpp.git &&
  cd prometheus-cpp/ && git submodule init && git submodule update
# download grpc and related components
cd ~/temp && version=1.16 && build=1 &&
  git clone https://github.com/grpc/grpc &&
  cd grpc && git fetch --all --tags --prune &&
  git checkout tags/v$version.$build -b v$version.$build &&
  git submodule update --init
# download and install pre-requisits for protobuf and grpc
apt-get install -y autoconf automake libtool curl make unzip libssl-dev
# download and install pre-requisits for rocksdb
apt-get install -y libgflags-dev libsnappy-dev zlib1g-dev libbz2-dev liblz4-dev libzstd-dev
# download rocksdb
cd ~/temp &&
  git clone https://github.com/facebook/rocksdb.git &&
  cd rocksdb &&
  git checkout v6.5.2 &&
  git submodule update --init --recursive
# download abseil
cd ~/temp &&
  git clone https://github.com/abseil/abseil-cpp.git &&
  cd abseil-cpp &&
  git checkout 20190808
# download and install tools for code coverage
apt-get install -y lcov
# download and install tools required by v18
apt-get install -y libcrypto++-dev &&
  apt-get install -y doxygen &&
  apt-get install -y python=2.7* &&
  apt-get install -y python-pip &&
  # Must use gcovr 3.2-1 as later version will create *.gcov files under tmp dir and remove them afterwards.
  # The consequence is we cannot upload these gcov files to sonarcube
  apt-get install -y gcovr=3.2-1
# download and install sqlite3
apt-get install -y sqlite3 libsqlite3-dev
# download and install boost
apt-get install -y libboost-all-dev
# download and install gettext (for envsubst)
apt-get install -y gettext
