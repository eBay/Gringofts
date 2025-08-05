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
bash "$SCRIPTS_DIR"/downloadDependencies.sh
bash "$SCRIPTS_DIR"/installDependencies.sh

set +x

ELAPSED=$SECONDS
printf 'Elapsed time: %02d:%02d:%02d\n' $((ELAPSED/3600)) $(( (ELAPSED%3600)/60 )) $((ELAPSED%60))

echo "Setup is done! Please proceed with gringofts and have fun."
