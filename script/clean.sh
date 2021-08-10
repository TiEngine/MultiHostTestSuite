#!/bin/bash

set -e
TOP_PATH=$(cd "$(dirname "$0")/.."; pwd)
echo ==== clean: delete the folder: ${TOP_PATH}/build ====

rm -rf ${TOP_PATH}/build

if test -n $1 ; then
if [ "$1" = "all" ] ; then
bash ${TOP_PATH}/TiRPC/script/clean.sh
fi
fi
