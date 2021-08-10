cmake_minimum_required(VERSION 3.10)
project(MultiHostTestSuit)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(../TiRPC/build/archive/TiRPC.cmake)

file(GLOB_RECURSE WORKER worker/*)
file(GLOB_RECURSE MANER maner/*)
file(GLOB_RECURSE ORDER order/*)

add_executable(worker ${WORKER})
add_executable(maner ${MANER})
add_executable(order ${ORDER})
