#!/bin/bash
# This script sets up the develop environment

ABSPATH=$(cd "${0%/*}" && echo "$PWD"/"${0##*/}")
SCRIPTS_DIR=$(dirname "$ABSPATH")

SECONDS=0

WORKING_DIR=$(pwd)
echo "working dir=$WORKING_DIR"


echo "Kicking off setup process, it will take a while, go and get a cup of coffee."

set -x

# Set up dependencies
if [[ "$OSTYPE" == "darwin"* ]]; then
  echo "MAC OS"
  bash "$SCRIPTS_DIR"/downloadDependencies-macos.sh
  bash "$SCRIPTS_DIR"/installDependencies-macos.sh
elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
  echo "Linux OS"
  # linux need root access
  EXECUTOR=$(whoami)
  if [[ "$EXECUTOR" != "root" ]]; then
    echo "Run this script with sudo as some operations need root access"
    exit 1
  fi
  bash "$SCRIPTS_DIR"/downloadDependencies.sh
  bash "$SCRIPTS_DIR"/installDependencies.sh
else
  echo "unknown OS"
fi
set +x

ELAPSED=$SECONDS
TZ=UTC0 printf 'Elapsed time: %(%H:%M:%S)T\n' "$ELAPSED"

echo "Setup is done! Please proceed with gringofts and have fun."
