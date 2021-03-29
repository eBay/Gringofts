#!/bin/sh
#./scripts/cleanForCMake.sh
set -e # exit on non-zero if build failed
# CXX=clang++-6.0 CC=clang-6.0 cmake -DCMAKE_BUILD_TYPE=Debug && \
CXX=g++-7 CC=gcc-7 cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS="-O2" -DCMAKE_C_FLAGS="-O2" &&
  echo "running cpplint" && make check &&
#  echo "building libetcdv3" && make libetcdv3 &&
  echo "building remaining targets" && make -j4
