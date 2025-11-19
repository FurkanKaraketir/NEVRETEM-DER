@echo off
cd /d "%~dp0"
set PATH=C:\Qt\6.9.1\mingw_64\bin;C:\Qt\Tools\mingw1310_64\bin;%PATH%
cd build\bin
StudentManager.exe
pause
