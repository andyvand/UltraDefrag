@echo off
echo Set Stefans' environment variables...

:: save username, since build process overwrites it
if not defined ORIG_USERNAME set ORIG_USERNAME=%USERNAME%

set WINSDKBASE=
set MINGWBASE=C:\PROGRA~1\MinGWStudio\MinGW
set MINGWx64BASE=
set WXWIDGETSDIR=
set NSISDIR=C:\Program Files\NSIS
set SEVENZIP_PATH=C:\Program Files\7-Zip

rem comment out next line to enable warnings to find unreachable code
goto :EOF

set CL=/Wall /wd4619 /wd4711 /wd4706
rem set LINK=/WX
