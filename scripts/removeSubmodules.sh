#!/bin/bash
# run this script under project's dir
set -x
# this script removes all dependent submodules
MODULES=("third_party/cpplint" "third_party/gtest" "third_party/inih" "third_party/spdlog" "third_party/abseil-cpp" "third_party/prometheus-cpp")

for MODULE in ${MODULES[*]}; do
  git submodule deinit "$MODULE"
  git rm --cached "$MODULE"
  rm -rf "$MODULE"
  rm -rf .git/modules/"$MODULE"
done

rm -rf third_party
git submodule sync
rm .gitmodules
