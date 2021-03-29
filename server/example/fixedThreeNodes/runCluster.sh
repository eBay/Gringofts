#!/bin/bash

WORKING_DIR=$(pwd)
echo "working dir=${WORKING_DIR}"

set -x

rm -rf node1 node2 node3
mkdir -p node1/snapshots node2/snapshots node3/snapshots

./build/ObjectStoreMain conf/kv_app_config1.ini > node1/log 2>&1 &
./build/ObjectStoreMain conf/kv_app_config2.ini > node2/log 2>&1 &
./build/ObjectStoreMain conf/kv_app_config3.ini > node3/log 2>&1 &
pgrep ObjectStoreMain
