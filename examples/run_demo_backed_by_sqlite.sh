#!/bin/bash

WORKING_DIR=$(pwd)
echo "working dir=${WORKING_DIR}"

set -x

rm -rf sqlite && mkdir -p sqlite/snapshots
# set up demo db
sqlite3 sqlite/demo.db -cmd ".read src/app_util/app.sql"
# type .exit in sqlite's prompt
./build/DemoApp conf/app_sqlite.ini > sqlite/log 2>&1 &
pgrep DemoApp