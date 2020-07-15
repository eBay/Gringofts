#!/bin/bash
# This script downloads all external dependencies

checkLastSuccess() {
  # shellcheck disable=SC2181
  if [[ $? -ne 0 ]]; then
    echo "Install Error: $1"
    exit 1
  fi
}

brew install cmake
brew install wget
brew install rocksdb
brew install boost
brew install openssl # (or brew link openssl to a version)
brew install lcov
brew install gcovr

mkdir ~/temp
# download grpc and related components
cd ~/temp && version=1.16 && build=1 &&
  git clone https://github.com/grpc/grpc &&
  cd grpc && git fetch --all --tags --prune &&
  git checkout tags/v$version.$build -b v$version.$build &&
  git submodule update --init
