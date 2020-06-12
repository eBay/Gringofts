#!/bin/bash
# This script downloads all external dependencies

checkLastSuccess() {
  # shellcheck disable=SC2181
  if [[ $? -ne 0 ]]; then
    echo "Install Error: $1"
    exit 1
  fi
}

mkdir ~/temp

# preparation
apt-get update -y &&
  apt-get install -y wget tar git build-essential apt-utils &&
  # To work around "E: The method driver /usr/lib/apt/methods/https could not be found." issue
  apt-get install -y apt-transport-https ca-certificates &&
  # download cmake 3.12
  cd ~/temp && version=3.12 && build=0 &&
  wget https://cmake.org/files/v$version/cmake-$version.$build.tar.gz &&
  tar -xzvf cmake-$version.$build.tar.gz &&
  mv cmake-$version.$build cmake
# download and install clang/clang++ 6.0.1
CLANG6=$(clang-6.0 --version | grep "6.0")
if [ -z "$CLANG6" ]; then
  wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key | apt-key add - &&
    apt-get install -y software-properties-common &&
    apt-add-repository "deb http://apt.llvm.org/xenial/ llvm-toolchain-xenial-6.0 main" &&
    apt-get update -y &&
    apt-get install -y clang-6.0
  checkLastSuccess "install clang 6.0 fails"
else
  echo "clang 6.0 has been installed, skip"
fi
# download and install gcc/g++ 7.4.0
GCC7=$(g++-7 --version | grep "7.4")
if [ -z "$GCC7" ]; then
  apt-get install -y software-properties-common &&
    add-apt-repository ppa:ubuntu-toolchain-r/test -y &&
    apt-get update -y &&
    apt-get install -y gcc-7 g++-7
  checkLastSuccess "install g++ 7.4 fails"
else
  echo "g++ 7.4 has been installed, skip"
fi
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
# download and install tools for code coverage
apt-get install -y lcov
# download and install tools required by gringofts
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
