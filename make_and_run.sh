#!/bin/sh

# full directory path containing this file
# https://stackoverflow.com/a/246128/4704639
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"


mkdir --verbose --parents build

set -o xtrace

cd build

cmake -DCMAKE_BUILD_TYPE=Debug -G Ninja ..

ninja

./epaper
