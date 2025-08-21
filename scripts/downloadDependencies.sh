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
mkdir -p "$HOME/temp"
OS_TYPE=$(uname)

# preparation
if [[ "$OS_TYPE" == "Darwin" ]]; then
  sudo -u "$(logname)" brew update &&
    sudo -u "$(logname)" brew install wget git make gcc
elif [[ "$OS_TYPE" == "Linux" ]]; then
  apt-get update -y &&
    apt-get install -y wget tar git build-essential apt-utils &&
    apt-get install -y apt-transport-https ca-certificates
else
  echo "Unsupported OS: $OS_TYPE"
  exit 1
fi

# download cmake
cmake_version=3.16
cd "$HOME/temp" && version=${cmake_version} && build=0 &&
wget https://cmake.org/files/v$version/cmake-$version.$build.tar.gz &&
tar -xzf cmake-$version.$build.tar.gz &&
mv cmake-$version.$build cmake
echo -e "\033[32mcmake ${cmake_version} downloaded successfully.\033[0m"

# download and install gcc/g++
gcc9_version=9.5.0
if [[ "$OS_TYPE" == "Linux" ]]; then
  GCC9=$(g++-9 --version | grep ${gcc9_version})
  if [ -z "$GCC9" ]; then
    apt-get install -y software-properties-common &&
      add-apt-repository ppa:ubuntu-toolchain-r/test -y &&
      apt-get update -y &&
      apt-get install -y gcc-9 g++-9
    checkLastSuccess "install g++ ${gcc9_version} fails"
    echo -e "\033[32mg++ ${gcc9_version} installed successfully.\033[0m"
  else
    echo "g++ ${gcc9_version} has been installed, skip"
  fi
fi

# download prometheus cpp client
prometheus_version="1.0.0"
if [[ "$OS_TYPE" == "Darwin" ]]; then
  sudo -u "$(logname)" brew install curl
  cd "$HOME/temp" || exit 1
  git clone -b "v${prometheus_version}" https://github.com/jupp0r/prometheus-cpp.git
  cd prometheus-cpp/ || exit 1
  git submodule update --init
  echo -e "\033[32mprometheus client ${prometheus_version} downloaded successfully.\033[0m"
elif [[ "$OS_TYPE" == "Linux" ]]; then
  apt-get install -y libcurl4-gnutls-dev &&
    cd "$HOME/temp" &&
    git clone -b v${prometheus_version} https://github.com/jupp0r/prometheus-cpp.git &&
    cd prometheus-cpp/ && git submodule init && git submodule update
  echo -e "\033[32mprometheus client ${prometheus_version} downloaded successfully.\033[0m"
else
  echo "Unsupported OS: $OS_TYPE"
  exit 1
fi

# download and install pre-requisites for protobuf and grpc
if [[ "$OS_TYPE" == "Darwin" ]]; then
  sudo -u "$(logname)" brew install autoconf automake libtool curl make unzip openssl
elif [[ "$OS_TYPE" == "Linux" ]]; then
  apt-get install -y autoconf automake libtool curl make unzip libssl-dev
else
  echo "Unsupported OS: $OS_TYPE"
  exit 1
fi

# download grpc and related components
grpc_version="1.24"
cd "$HOME/temp" && version=${grpc_version} && build=1 &&
  git clone https://github.com/grpc/grpc &&
  cd grpc && git fetch --all --tags --prune &&
  git checkout tags/v$version.$build -b v$version.$build &&
  git submodule update --init
echo -e "\033[32mgrpc ${grpc_version} downloaded successfully.\033[0m"

# download and install pre-requisites for rocksdb
if [[ "$OS_TYPE" == "Darwin" ]]; then
  sudo -u "$(logname)" brew install gflags snappy zlib bzip2 lz4 zstd
elif [[ "$OS_TYPE" == "Linux" ]]; then
  apt-get install -y libgflags-dev libsnappy-dev zlib1g-dev libbz2-dev liblz4-dev libzstd-dev
else
  echo "Unsupported OS: $OS_TYPE"
  exit 1
fi

# download rocksdb
rocksdb_version="8.9.1"
cd "$HOME/temp" &&
  git clone https://github.com/facebook/rocksdb.git &&
  cd rocksdb &&
  git checkout v${rocksdb_version} &&
  git submodule update --init --recursive
echo -e "\033[32mrocksdb ${rocksdb_version} downloaded successfully.\033[0m"

# download abseil
abseil_version="20250512.1"
cd "$HOME/temp" &&
  git clone https://github.com/abseil/abseil-cpp.git &&
  cd abseil-cpp &&
  git checkout ${abseil_version}
echo -e "\033[32mabseil-cpp ${abseil_version} downloaded successfully.\033[0m"

# download and install tools for code coverage
if [[ "$OS_TYPE" == "Darwin" ]]; then
  sudo -u "$(logname)" brew install lcov
  echo -e "\033[32mlcov installed successfully.\033[0m"
elif [[ "$OS_TYPE" == "Linux" ]]; then
  apt-get install -y lcov
  echo -e "\033[32mlcov installed successfully.\033[0m"
else
  echo "Unsupported OS: $OS_TYPE"
  exit 1
fi

# download and install tools required by gringofts
if [[ "$OS_TYPE" == "Darwin" ]]; then
  sudo -u "$(logname)" brew install cryptopp doxygen gcovr
  sudo -u "$(logname)" brew install sqlite3
  sudo -u "$(logname)" brew install boost
  sudo -u "$(logname)" brew install gettext
  echo -e "\033[32mtools required by gringofts installed successfully.\033[0m"
elif [[ "$OS_TYPE" == "Linux" ]]; then
  apt-get install -y libcrypto++-dev &&
    apt-get install -y doxygen &&
    apt-get install -y python2 &&
    apt-get install -y gcovr
  # download and install sqlite3
  apt-get install -y sqlite3 libsqlite3-dev
  # download and install boost
  apt-get install -y libboost-all-dev
  # download and install gettext (for envsubst)
  apt-get install -y gettext
  echo -e "\033[32mtools required by gringofts installed successfully.\033[0m"
else
  echo "Unsupported OS: $OS_TYPE"
  exit 1
fi
