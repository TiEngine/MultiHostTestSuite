cd %~dp0..

set BUILD_TYPE=%1
set BUILD_BITS=%2
set BUILD_COMPILE=%3

rem script params: msvc.bat [BUILD_TYPE] [BUILD_COMPILE] [BUILD_BITS]
rem default value: BUILD_TYPE=Debug BUILD_COMPILE=VS2015 BUILD_BITS=x86

if "%BUILD_TYPE%"=="" (set BUILD_TYPE=Debug)
if "%BUILD_COMPILE%"=="" (set BUILD_COMPILE=VS2015)

if "%BUILD_COMPILE%"=="VS2015" (set BUILD_COMPILE_FULL_NAME=Visual Studio 14 2015)
if "%BUILD_COMPILE%"=="VS2017" (set BUILD_COMPILE_FULL_NAME=Visual Studio 15 2017)
if "%BUILD_COMPILE%"=="VS2019" (set BUILD_COMPILE_FULL_NAME=Visual Studio 16 2019)

rem only VS2019 uses "BUILD_ARCH_ARG"
set BUILD_ARCH_ARG=
if "%BUILD_COMPILE%"=="Visual Studio 16 2019" (
if "%BUILD_BITS%"=="x64" (set BUILD_ARCH_ARG=-A x64) ^
else (set BUILD_ARCH_ARG=-A Win32)
) else (
if "%BUILD_BITS%"=="x64" (set BUILD_COMPILE_FULL_NAME=%BUILD_COMPILE_FULL_NAME% Win64)
)

if exist "TiRPC\build\archive" (goto _tag_build_trace_src)

:_tag_build_trace_tirpc
rem ===== build trace <tirpc> =====

cd TiRPC
call script\build_msvc.bat %BUILD_TYPE% %BUILD_BITS% %BUILD_COMPILE%
cd ..

:_tag_build_trace_src
rem ===== build trace <src> =====

mkdir build\solution
cd build\solution
cmake -G "%BUILD_COMPILE_FULL_NAME%" %BUILD_ARCH_ARG% ../../TestSuit
cmake --build . --config %BUILD_TYPE% -j 8
cd ..\..

:_tag_archive
rem ===== archive =====

mkdir build\archive
cd build\archive

mkdir licenses\tirpc
copy ..\..\TiRPC\build\archive\License.txt licenses\tirpc.txt /Y
copy ..\..\TiRPC\build\archive\licenses\zeromq.txt licenses\tirpc\zeromq.txt /Y
copy ..\..\TiRPC\build\archive\licenses\cppzmq.txt licenses\tirpc\cppzmq.txt /Y
copy ..\..\TiRPC\build\archive\licenses\cereal.txt licenses\tirpc\cereal.txt /Y
copy ..\..\LICENSE.txt .\License.txt /Y

copy ..\..\TiRPC\build\archive\libraries\libzmq.dll . /Y
copy ..\..\build\solution\%BUILD_TYPE%\*.exe /Y

cd ..\..
