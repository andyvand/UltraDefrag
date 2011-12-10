@echo off
echo Set Stefans' environment variables...

:: save username, since build process overwrites it
if not defined ORIG_USERNAME set ORIG_USERNAME=%USERNAME%

set WINDDKBASE=C:\WinDDK\3790.1830
set WINSDKBASE=C:\Program Files\Microsoft SDKs\Windows\v7.1
set MINGWBASE=
set MINGWx64BASE=
set NETRUNTIMEPATH=C:\WINDOWS\Microsoft.NET\Framework\v2.0.50727
set NSISDIR=C:\Program Files (x86)\NSIS
set SEVENZIP_PATH=C:\Program Files\7-Zip
set MSVSBIN=
set DDKINCDIR=
set PELLESC_BASE=C:\Program Files\PellesC

rem comment out next line to enable warnings to find unreachable code
goto :EOF

set CL=/Wall /wd4619 /wd4711 /wd4706
rem set LINK=/WX
