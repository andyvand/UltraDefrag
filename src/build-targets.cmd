@echo off

::
:: This script builds all binaries for UltraDefrag project
:: for all of the supported target platforms.
:: Copyright (c) 2007-2011 by Dmitri Arkhangelski (dmitriar@gmail.com).
:: Copyright (c) 2010-2011 by Stefan Pendl (stefanpe@users.sourceforge.net).
::
:: This program is free software; you can redistribute it and/or modify
:: it under the terms of the GNU General Public License as published by
:: the Free Software Foundation; either version 2 of the License, or
:: (at your option) any later version.
::
:: This program is distributed in the hope that it will be useful,
:: but WITHOUT ANY WARRANTY; without even the implied warranty of
:: MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
:: GNU General Public License for more details.
::
:: You should have received a copy of the GNU General Public License
:: along with this program; if not, write to the Free Software
:: Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
::

rem Usage:
rem     build-targets [<compiler>]
rem
rem Available <compiler> values:
rem     --use-mingw (default)
rem     --use-msvc
rem     --use-winddk
rem     --use-winsdk
rem     --use-mingw-x64 (experimental)
rem
rem Skip any processor architecture to reduce compile time
rem     --no-x86
rem     --no-amd64
rem     --no-ia64

rem NOTE: IA-64 targeting binaries were never tested by the authors 
rem due to missing appropriate hardware and appropriate 64-bit version 
rem of Windows.

call ParseCommandLine.cmd %*

:: create all directories required to store target binaries
mkdir lib
mkdir lib\amd64
mkdir lib\ia64
mkdir bin
mkdir bin\amd64
mkdir bin\ia64

:: copy source files to obj directory
xcopy /I /Y /Q .\bootexctrl  .\obj\bootexctrl
xcopy /I /Y /Q .\hibernate   .\obj\hibernate
xcopy /I /Y /Q .\console     .\obj\console
xcopy /I /Y /Q .\native      .\obj\native
xcopy /I /Y /Q .\include     .\obj\include
xcopy /I /Y /Q .\share       .\obj\share
xcopy /I /Y /Q .\dll\udefrag .\obj\udefrag
xcopy /I /Y /Q .\dll\zenwinx .\obj\zenwinx
xcopy /I /Y /Q .\gui         .\obj\gui
xcopy /I /Y /Q .\gui\res     .\obj\gui\res
xcopy /I /Y /Q .\lua5.1      .\obj\lua5.1
xcopy /I /Y /Q .\dll\wgx     .\obj\wgx

:: copy external files on which udefrag.exe command line tool depends
copy /Y .\obj\share\*.* .\obj\console\

:: copy header files to different locations
:: to make relative paths of them the same
:: as in /src directory
mkdir obj\dll
mkdir obj\dll\udefrag
copy /Y obj\udefrag\udefrag.h obj\dll\udefrag
mkdir obj\dll\wgx
copy /Y obj\wgx\wgx.h obj\dll\wgx
mkdir obj\dll\zenwinx
copy /Y obj\zenwinx\*.h obj\dll\zenwinx\

:: let's build all modules by selected compiler
if %UD_BLD_FLG_USE_COMPILER% equ 0 (
    echo No parameters specified, using defaults.
    goto mingw_build
)

if %UD_BLD_FLG_USE_COMPILER% equ %UD_BLD_FLG_USE_WINDDK%  goto ddk_build
if %UD_BLD_FLG_USE_COMPILER% equ %UD_BLD_FLG_USE_MSVC%    goto msvc_build
if %UD_BLD_FLG_USE_COMPILER% equ %UD_BLD_FLG_USE_MINGW%   goto mingw_build
if %UD_BLD_FLG_USE_COMPILER% equ %UD_BLD_FLG_USE_MINGW64% goto mingw_x64_build
if %UD_BLD_FLG_USE_COMPILER% equ %UD_BLD_FLG_USE_WINSDK%  goto winsdk_build


:ddk_build

    set BUILD_ENV=winddk
    set OLD_PATH=%path%

    :: check if we are using WDK 6 and above
    for /d %%D in ( "%WINDDKBASE%" ) do set UD_DDK_NAME=%%~nD
    set UD_DDK_VER=%UD_DDK_NAME:~0,4%
    set UD_DDK_NAME=
    echo UD_DDK_VER set to "%UD_DDK_VER%"... 

    rem workaround for WDK 6 and above
    if %UD_DDK_VER% NEQ 3790 set IGNORE_LINKLIB_ABUSE=1

    if %UD_BLD_FLG_BUILD_X86% neq 0 (
        echo --------- Target is x86 ---------
        set AMD64=
        set IA64=
        pushd ..
        call "%WINDDKBASE%\bin\setenv.bat" %WINDDKBASE% fre WNET
        popd
        set BUILD_DEFAULT=-nmake -i -g -P
        set UDEFRAG_LIB_PATH=..\..\lib
        call :build_modules X86 || exit /B 1
    )
    
    set path=%OLD_PATH%
    set DDKBUILDENV=
    
    if %UD_BLD_FLG_BUILD_AMD64% neq 0 (
        echo --------- Target is x64 ---------
        set IA64=
        pushd ..
        rem WDK 6 and above use x64 instead of AMD64
        if %UD_DDK_VER% NEQ 3790 (
            call "%WINDDKBASE%\bin\setenv.bat" %WINDDKBASE% fre x64 WNET
        ) else (
            call "%WINDDKBASE%\bin\setenv.bat" %WINDDKBASE% fre AMD64 WNET
        )
        popd
        set BUILD_DEFAULT=-nmake -i -g -P
        set UDEFRAG_LIB_PATH=..\..\lib\amd64
        call :build_modules amd64 || exit /B 1
    )
    
    set path=%OLD_PATH%
    set DDKBUILDENV=
    
    if %UD_BLD_FLG_BUILD_IA64% neq 0 (
        echo --------- Target is ia64 ---------
        set AMD64=
        pushd ..
        call "%WINDDKBASE%\bin\setenv.bat" %WINDDKBASE% fre 64 WNET
        popd
        set BUILD_DEFAULT=-nmake -i -g -P
        set UDEFRAG_LIB_PATH=..\..\lib\ia64
        call :build_modules ia64 || exit /B 1
    )
    
    goto ddk_build_success
    
    :ddk_build_fail
    set path=%OLD_PATH%
    set OLD_PATH=
    set DDKBUILDENV=
    set UD_DDK_VER=
    set IGNORE_LINKLIB_ABUSE=
    exit /B 1
    
    :ddk_build_success
    set path=%OLD_PATH%
    set OLD_PATH=
    set DDKBUILDENV=
    set UD_DDK_VER=
    set IGNORE_LINKLIB_ABUSE=
    
exit /B 0


:winsdk_build

    set BUILD_ENV=winsdk
    set OLD_PATH=%path%

    if %UD_BLD_FLG_BUILD_X86% neq 0 (
        echo --------- Target is x86 ---------
        set AMD64=
        set IA64=
        pushd ..
        call "%WINSDKBASE%\bin\SetEnv.Cmd" /Release /x86 /xp
        popd
        set UDEFRAG_LIB_PATH=..\..\lib
        call :build_modules X86 || exit /B 1
    )
    
    set path=%OLD_PATH%

    if %UD_BLD_FLG_BUILD_AMD64% neq 0 (
        echo --------- Target is x64 ---------
        set IA64=
        set AMD64=1
        pushd ..
        call "%WINSDKBASE%\bin\SetEnv.Cmd" /Release /x64 /xp
        popd
        set UDEFRAG_LIB_PATH=..\..\lib\amd64
        call :build_modules amd64 || exit /B 1
    )
    
    set path=%OLD_PATH%

    if %UD_BLD_FLG_BUILD_IA64% neq 0 (
        echo --------- Target is ia64 ---------
        set AMD64=
        set IA64=1
        pushd ..
        call "%WINSDKBASE%\bin\SetEnv.Cmd" /Release /ia64 /xp
        popd
        set BUILD_DEFAULT=-nmake -i -g -P
        set UDEFRAG_LIB_PATH=..\..\lib\ia64
        call :build_modules ia64 || exit /B 1
    )
    
    set path=%OLD_PATH%
    set OLD_PATH=
    
exit /B 0


:msvc_build

    set OLD_PATH=%path%

    call "%MSVSBIN%\vcvars32.bat"
    set BUILD_ENV=msvc
    set UDEFRAG_LIB_PATH=..\..\lib
    call :build_modules X86 || exit /B 1

    set path=%OLD_PATH%
    set OLD_PATH=

exit /B 0


:mingw_x64_build

    set OLD_PATH=%path%

    echo --------- Target is x64 ---------
    set AMD64=1
    set path=%MINGWx64BASE%\bin;%path%
    set BUILD_ENV=mingw_x64
    set UDEFRAG_LIB_PATH=..\..\lib\amd64
    call :build_modules amd64 || exit /B 1

    set path=%OLD_PATH%
    set OLD_PATH=

exit /B 0


:mingw_build

    set OLD_PATH=%path%

    set path=%MINGWBASE%\bin;%path%
    set BUILD_ENV=mingw
    set UDEFRAG_LIB_PATH=..\..\lib
    call :build_modules X86 || exit /B 1

    set path=%OLD_PATH%
    set OLD_PATH=

exit /B 0


:: Builds all UltraDefrag modules
:: Example: call :build_modules X86
:build_modules
    rem update manifests
    call make-manifests.cmd %1 || exit /B 1

    rem rebuild modules
    set UD_BUILD_TOOL=lua ..\..\tools\mkmod.lua
    
    :: monolithic native executable can be
    :: produced currently by DDK only
    if %UD_BLD_FLG_USE_COMPILER% equ %UD_BLD_FLG_USE_WINDDK% (
        echo Compile monolithic native interface...
        pushd obj\zenwinx
        %UD_BUILD_TOOL% zenwinx.build static-lib || goto fail
        cd ..\udefrag
        %UD_BUILD_TOOL% udefrag.build static-lib || goto fail
        cd ..\native
        %UD_BUILD_TOOL% defrag_native.build || goto fail

        echo Compile native DLL's...
        cd ..\zenwinx
        %UD_BUILD_TOOL% zenwinx.build || goto fail
        cd ..\udefrag
        %UD_BUILD_TOOL% udefrag.build || goto fail
    ) else (
        pushd obj\zenwinx
        %UD_BUILD_TOOL% zenwinx.build || goto fail
        cd ..\udefrag
        %UD_BUILD_TOOL% udefrag.build || goto fail
        cd ..\native
        %UD_BUILD_TOOL% defrag_native.build || goto fail
    )

    rem workaround for WDK 6 and above
    if "%UD_DDK_VER%" NEQ "3790" (
        set OLD_CL=%CL% & set CL=/QIfist %CL%
    )

    cd ..\lua5.1
    %UD_BUILD_TOOL% lua5.1a_dll.build || goto fail
    %UD_BUILD_TOOL% lua5.1a_exe.build || goto fail
    %UD_BUILD_TOOL% lua5.1a_gui.build || goto fail

    rem workaround for WDK 6 and above
    if "%UD_DDK_VER%" NEQ "3790" (
        set CL=%OLD_CL% & set OLD_CL=
    )

    cd ..\wgx
    %UD_BUILD_TOOL% wgx.build ||  goto fail

    echo Compile other modules...
    cd ..\bootexctrl
    %UD_BUILD_TOOL% bootexctrl.build || goto fail
    cd ..\hibernate
    %UD_BUILD_TOOL% hibernate.build || goto fail
    cd ..\console
    %UD_BUILD_TOOL% defrag.build || goto fail

    cd ..\gui
    %UD_BUILD_TOOL% ultradefrag.build && goto success

    :fail
    set UD_BUILD_TOOL=
    popd
    exit /B 1
    
    :success
    set UD_BUILD_TOOL=
    popd

exit /B 0
