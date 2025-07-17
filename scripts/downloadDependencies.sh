#!/bin/bash
# This script downloads all external dependencies

checkLastSuccess() {
  # shellcheck disable=SC2181
  if [[ $? -ne 0 ]]; then
    echo "Install Error: $1"
    exit 1
  fi
}

set +x
mkdir ~/temp

# preparation
apt-get update -y &&
  apt-get install -y wget tar git build-essential apt-utils &&
  apt-get install -y apt-transport-https ca-certificates
# download cmake
cmake_version=3.14
cd ~/temp && version=${cmake_version} && build=0 &&
wget https://cmake.org/files/v$version/cmake-$version.$build.tar.gz &&
tar -xzf cmake-$version.$build.tar.gz &&
mv cmake-$version.$build cmake
# download and install gcc/g++
gcc9_version=9.5.0
GCC9=$(g++-9 --version | grep ${gcc9_version})
if [ -z "$GCC9" ]; then
  apt-get install -y software-properties-common &&
    add-apt-repository ppa:ubuntu-toolchain-r/test -y &&
    apt-get update -y &&
    apt-get install -y gcc-9 g++-9
  checkLastSuccess "install g++ ${gcc9_version} fails"
else
  echo "g++ ${gcc9_version} has been installed, skip"
fi
# download prometheus cpp client
prometheus_version="1.0.0"
apt-get install -y libcurl4-gnutls-dev &&
  cd ~/temp &&
  git clone -b v${prometheus_version} https://github.com/jupp0r/prometheus-cpp.git &&
  cd prometheus-cpp/ && git submodule init && git submodule update
# download grpc and related components
grpc_version="1.16"
cd ~/temp && version=${grpc_version} && build=1 &&
  git clone https://github.com/grpc/grpc &&
  cd grpc && git fetch --all --tags --prune &&
  git checkout tags/v$version.$build -b v$version.$build &&
  git submodule update --init
# download and install pre-requisites for protobuf and grpc
apt-get install -y autoconf automake libtool curl make unzip libssl-dev
# download and install pre-requisites for rocksdb
apt-get install -y libgflags-dev libsnappy-dev zlib1g-dev libbz2-dev liblz4-dev libzstd-dev
# download rocksdb
rocksdb_version="6.5.2"
cd ~/temp &&
  git clone https://github.com/facebook/rocksdb.git &&
  cd rocksdb &&
  git checkout v${rocksdb_version} &&
  git submodule update --init --recursive
# download and install tools for code coverage
apt-get install -y lcov
# download and install tools required by gringofts
apt-get install -y libcrypto++-dev &&
  apt-get install -y doxygen &&
  apt-get install -y python2 &&
  apt-get install -y python-pip &&
  # Must use gcovr 3.2-1 as later version will create *.gcov files under tmp dir and remove them afterwards.
  # The consequence is we cannot upload these gcov files to sonarcube
  apt-get install -y gcovr=3.4-1
# download and install sqlite3
apt-get install -y sqlite3 libsqlite3-dev
# download and install boost
apt-get install -y libboost-all-dev
# download and install gettext (for envsubst)
apt-get install -y gettext
