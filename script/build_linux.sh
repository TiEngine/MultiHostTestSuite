#!/bin/bash

set -e
TOP_PATH=$(cd "$(dirname "$0")/.."; pwd)

# See ../build.sh
BUILD_TYPE="$1"
BUILD_BITS="$2"

if test -z ${BUILD_TYPE} ; then
BUILD_TYPE="Debug"
fi

if test -z ${BUILD_BITS} ; then
BUILD_BITS="Undefined"
fi

BUILD_JOBS=$(($(grep -c ^processor /proc/cpuinfo) - 1))

# The building in linux will use the default compile toolchain in the system.
echo ==== build: building MultiHostTestSuit now. ====

echo Build type: ${BUILD_TYPE}
echo Build bits: ${BUILD_BITS}
if [ ${BUILD_BITS} = "x64" ] ; then
export CFLAGS="-m64 ${CFLAGS}"
export CXXFLAGS="-m64 ${CXXFLAGS}"
elif [ ${BUILD_BITS} = "x86" ] ; then
export CFLAGS="-m32 ${CFLAGS}"
export CXXFLAGS="-m32 ${CXXFLAGS}"
else
echo Undefined or unknown build bit: ${BUILD_BITS}, use the default.
fi

export MAKEFLAGS="${MAKEFLAGS} -j${BUILD_JOBS}"
echo Build jobs count: ${BUILD_JOBS}

echo Build Config Infos of Env:
echo - CFLAGS:    ${CFLAGS}
echo - CXXFLAGS:  ${CXXFLAGS}
echo - MAKEFLAGS: ${MAKEFLAGS}

echo =================================================
echo +++++++++++++++  BUILD SUBMODULE  +++++++++++++++
if [ ! -d "${TOP_PATH}/TiRPC/build/archive" ] ; then
echo Building submodule of TiRPC...
bash ${TOP_PATH}/TiRPC/script/build_linux.sh ${BUILD_TYPE} ${BUILD_BITS}
else
echo Submodule already built.
fi
echo +++++++++++++++  BUILD SUBMODULE  +++++++++++++++
echo =================================================

echo Building MultiHostTestSuit...
mkdir -p build/solution
cd build/solution
cmake -DCMAKE_TOOLCHAIN_FILE=${TOP_PATH}/cmake/qnx.cmake ../../TestSuit
cmake --build . --config ${BUILD_TYPE}
cd ../..

echo Archiving...
mkdir -p build/archive
cd build/archive
echo - Copy runtime files
cp -u ../../TiRPC/build/archive/libraries//libzmq.so .
cp -u ../../build/solution/mhts_* .
echo - Copy license files
mkdir -p licenses/tirpc
cp -u ../../TiRPC/build/archive/License.txt licenses/tirpc.txt
cp -u ../../TiRPC/build/archive/licenses/zeromq.txt licenses/tirpc/zeromq.txt
cp -u ../../TiRPC/build/archive/licenses/cppzmq.txt licenses/tirpc/cppzmq.txt
cp -u ../../TiRPC/build/archive/licenses/cereal.txt licenses/tirpc/cereal.txt
cp -u ../../LICENSE.txt ./License.txt
cd ../..

echo ====================
echo Build successfully!!
echo ====================
