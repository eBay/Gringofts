#!/bin/bash

WORKING_DIR=$(pwd)
echo "working dir=${WORKING_DIR}"

set -x

rm -rf node_1 node_2 node_3
mkdir -p node_1/snapshots node_2/snapshots node_3/snapshots

./build/DemoApp conf/app_raft_1.ini > node_1/log 2>&1 &
./build/DemoApp conf/app_raft_2.ini > node_2/log 2>&1 &
./build/DemoApp conf/app_raft_3.ini > node_3/log 2>&1 &
pgrep DemoApp