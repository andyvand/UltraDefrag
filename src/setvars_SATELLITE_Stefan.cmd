@echo off
echo Set Stefans' environment variables...

:: save username, since build process overwrites it
if not defined ORIG_USERNAME set ORIG_USERNAME=%USERNAME%

:: set WINDDKBASE=C:\WinDDK\3790.1830
set WINDDKBASE=C:\WINDDK\7600.16385.1
set WINSDKBASE=
set MINGWBASE=C:\MinGW32
set MINGWx64BASE=
set NSISDIR=C:\Program Files (x86)\NSIS
set SEVENZIP_PATH=C:\Program Files\7-Zip

rem comment out next line to enable warnings to find unreachable code
goto :EOF

set CL=/Wall /wd4619 /wd4711 /wd4706
rem set LINK=/WX
