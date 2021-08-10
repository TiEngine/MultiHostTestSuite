#!/bin/bash

set -e
TOP_PATH=$(cd "$(dirname "$0")/.."; pwd)

bash ${TOP_PATH}/script/clean.sh all
bash ${TOP_PATH}/script/build_linux.sh Release

mkdir -p ${TOP_PATH}/artifact
cp -ruv ${TOP_PATH}/build/archive/* ${TOP_PATH}/artifact
