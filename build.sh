#!/bin/bash

set -e
CURRENT_PATH=$(cd "$(dirname "$0")"; pwd)
echo [MultiHostTestSuit] Current path is: ${CURRENT_PATH}

BUILD_QNX="$1"  # ON, OFF
RUN_MODE="$2"   # Command

BUILD_TYPE="$3" # Debug, Rlease, ...
BUILD_BITS="$4" # x64, x86

if test -z ${RUN_MODE} ; then
RUN_MODE="man"
fi

echo Config Infos:
echo - BUILD_QNX: ${BUILD_QNX}
echo - RUN_MODE: ${RUN_MODE}
echo - BUILD_TYPE: ${BUILD_TYPE}
echo - BUILD_BITS: ${BUILD_BITS}

if [ ${BUILD_QNX} = "ON" ] ; then
    export BUILD_QNX=${BUILD_QNX}
fi

if [ ${RUN_MODE} = "man" ] ; then
echo Build by manual with default configuration, and release mode.
bash ${CURRENT_PATH}/script/build_linux.sh Release
elif [ ${RUN_MODE} = "linux" ] ; then
bash ${CURRENT_PATH}/script/build_linux.sh ${BUILD_TYPE} ${BUILD_BITS}
elif [ ${RUN_MODE} = "clean" ] ; then
bash ${CURRENT_PATH}/script/clean.sh ${BUILD_TYPE}
elif [ ${RUN_MODE} = "config" ] ; then
bash ${CURRENT_PATH}/script/config.sh
elif [ ${RUN_MODE} = "ci" ] ; then
bash ${CURRENT_PATH}/script/ci.sh
else
echo Unknown run mode: ${RUN_MODE}
fi

if [ ${RUN_MODE} != "ci" ] ; then
bash ${CURRENT_PATH}/script/pause.sh
fi
