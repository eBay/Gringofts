#!/bin/bash

WORKING_DIR=$(pwd)
echo "working dir=${WORKING_DIR}"

set -x

rm -rf node
mkdir -p node/snapshots

./build/ObjectStoreMain conf/kv_app_config.ini > node/log 2>&1 &
pgrep ObjectStoreMain
