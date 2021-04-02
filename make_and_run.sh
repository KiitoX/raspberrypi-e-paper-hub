#!/bin/bash

# full directory path containing this file
# https://stackoverflow.com/a/246128/4704639
cd "$( dirname "${BASH_SOURCE[0]}" )" || exit

mkdir --verbose --parents build

cd build || exit

pwd

set -o xtrace

cmake -DCMAKE_BUILD_TYPE="Debug" -G Ninja .. || exit

ninja || exit

cd ..
sudo gdb build/epaper -ex run
