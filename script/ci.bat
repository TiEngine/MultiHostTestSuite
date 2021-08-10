cd %~dp0

call clean.bat all
call build_msvc.bat Release x64 VS2015
rem build_msvc will change the working path to .\..

mkdir artifact
xcopy build\archive artifact /E /Y
