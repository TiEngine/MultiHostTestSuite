set(QNX_HOST /home/dds-qnx/qnx710/host/linux/x86_64)

set(CMAKE_SYSTEM_NAME QNX)

set(ntoarch aarch64)
set(arch gcc_ntoaarch64le)

set(QNX_PROCESSOR aarch64)

set(CMAKE_C_COMPILER ${QNX_HOST}/usr/bin/qcc)
set(CMAKE_C_COMPILER_TARGET ${arch})

set(CMAKE_CXX_COMPILER ${QNX_HOST}/usr/bin/q++)
set(CMAKE_CXX_COMPILER_TARGET ${arch})

set(CMAKE_ASM_COMPILER qcc -V${arch})
set(CMAKE_ASM_DEFINE_FLAG "-Wa,--defsym,")

set(CMAKE_RANLIB ${QNX_HOST}/usr/bin/nto${ntoarch}-ranlib
    CACHE PATH "QNX ranlib Program" FORCE)
set(CMAKE_AR ${QNX_HOST}/usr/bin/nto${ntoarch}-ar
    CACHE PATH "QNX ar Program" FORCE)
set(CMAKE_LINKER ${QNX_HOST}/usr/bin/nto${ntoarch}-ld
    CACHE PATH "QNX ld Program" FORCE)

add_definitions(-D_XOPEN_SOURCE=700)
