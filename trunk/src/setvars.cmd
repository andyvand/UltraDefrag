@echo off
echo Set common environment variables...

:: UltraDefrag version
set VERSION_MAJOR=7
set VERSION_MINOR=0
set VERSION_REVISION=0

:: alpha1, beta2, rc3 etc.
:: unset for the final releases
set RELEASE_STAGE=beta1

:: paths to development tools
set WINSDKBASE=C:\Program Files\Microsoft SDKs\Windows\v6.1
set MINGWBASE=D:\Software\MinGWStudio\MinGW
set MINGWx64BASE=D:\Software\mingw64
set WXWIDGETSDIR=D:\Software\wxMSW-2.8.12
set NSISDIR=D:\Software\Tools\NSIS
set SEVENZIP_PATH=C:\Program Files\7-Zip
set GNUWIN32_DIR=D:\Software\GnuWin32\bin
set CODEBLOCKS_EXE=D:\Software\CodeBlocks\codeblocks.exe

:: auxiliary stuff
set VERSION=%VERSION_MAJOR%,%VERSION_MINOR%,%VERSION_REVISION%,0
set VERSION2="%VERSION_MAJOR%, %VERSION_MINOR%, %VERSION_REVISION%, 0\0"
set ULTRADFGVER=%VERSION_MAJOR%.%VERSION_MINOR%.%VERSION_REVISION%
if "%RELEASE_STAGE%" neq "" (
    set UDVERSION_SUFFIX=%ULTRADFGVER%-%RELEASE_STAGE%
) else (
    set UDVERSION_SUFFIX=%ULTRADFGVER%
)
