set CLEAN_PATH=%~dp0..
set CLEAN_SCOPE=%1

if "%CLEAN_SCOPE%"=="all" (call %CLEAN_PATH%\TiRPC\script\clean.bat)

rd %CLEAN_PATH%\build /S /Q
