@echo off

::
:: This script builds all binaries for UltraDefrag project
:: for all of the supported target platforms.
:: Copyright (c) 2007-2013 Dmitri Arkhangelski (dmitriar@gmail.com).
:: Copyright (c) 2010-2013 Stefan Pendl (stefanpe@users.sourceforge.net).
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
rem     --use-mingw     (default)
rem     --use-winsdk    (we use it for official releases)
rem     --use-mingw-x64 (experimental, produces wrong x64 code)
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
(
    echo doxyfile
    echo .dox
    echo .html
    echo .mdsp
    echo .cbp
    echo .depend
    echo .layout
) >"%~n0_exclude.txt"

xcopy .\bootexctrl  .\obj\bootexctrl  /I /Y /Q /EXCLUDE:%~n0_exclude.txt
xcopy .\console     .\obj\console     /I /Y /Q /EXCLUDE:%~n0_exclude.txt
xcopy .\console\res .\obj\console\res /I /Y /Q /EXCLUDE:%~n0_exclude.txt
xcopy .\dll\udefrag .\obj\udefrag     /I /Y /Q /EXCLUDE:%~n0_exclude.txt
xcopy .\dll\wgx     .\obj\wgx         /I /Y /Q /EXCLUDE:%~n0_exclude.txt
xcopy .\dll\zenwinx .\obj\zenwinx     /I /Y /Q /EXCLUDE:%~n0_exclude.txt
xcopy .\gui         .\obj\gui         /I /Y /Q /EXCLUDE:%~n0_exclude.txt
xcopy .\gui\res     .\obj\gui\res     /I /Y /Q /EXCLUDE:%~n0_exclude.txt
xcopy .\wxgui       .\obj\wxgui       /I /Y /Q /EXCLUDE:%~n0_exclude.txt
xcopy .\wxgui\res   .\obj\wxgui\res   /I /Y /Q /EXCLUDE:%~n0_exclude.txt
xcopy .\hibernate   .\obj\hibernate   /I /Y /Q /EXCLUDE:%~n0_exclude.txt
xcopy .\include     .\obj\include     /I /Y /Q /EXCLUDE:%~n0_exclude.txt
xcopy .\lua5.1      .\obj\lua5.1      /I /Y /Q /EXCLUDE:%~n0_exclude.txt
xcopy .\lua         .\obj\lua         /I /Y /Q /EXCLUDE:%~n0_exclude.txt
xcopy .\lua-gui     .\obj\lua-gui     /I /Y /Q /EXCLUDE:%~n0_exclude.txt
xcopy .\native      .\obj\native      /I /Y /Q /EXCLUDE:%~n0_exclude.txt
xcopy .\share       .\obj\share       /I /Y /Q /EXCLUDE:%~n0_exclude.txt

del /f /q "%~n0_exclude.txt"

:: copy external files on which udefrag.exe command line tool depends
copy /Y .\obj\share\*.* .\obj\console\

:: copy external files on which monolithic native interface depends
copy /Y .\obj\native\udefrag.c .\obj\native\udefrag-native.c
copy /Y .\obj\udefrag\*.* .\obj\native\
del /Q .\obj\native\udefrag.rc
copy /Y .\obj\native\volume.c .\obj\native\udefrag-volume.c
copy /Y .\obj\zenwinx\*.* .\obj\native\
del /Q .\obj\native\zenwinx.rc

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

:: build list of headers to produce dependencies
:: for MinGW/SDK makefiles from
cd obj
dir /S /B *.h >headers || exit /B 1
copy /Y .\headers .\bootexctrl || exit /B 1
copy /Y .\headers .\console    || exit /B 1
copy /Y .\headers .\udefrag    || exit /B 1
copy /Y .\headers .\wgx        || exit /B 1
copy /Y .\headers .\zenwinx    || exit /B 1
copy /Y .\headers .\gui        || exit /B 1
copy /Y .\headers .\wxgui      || exit /B 1
copy /Y .\headers .\hibernate  || exit /B 1
copy /Y .\headers .\lua5.1     || exit /B 1
copy /Y .\headers .\lua        || exit /B 1
copy /Y .\headers .\lua-gui    || exit /B 1
copy /Y .\headers .\native     || exit /B 1
cd ..

:: let's build all modules by selected compiler
if %UD_BLD_FLG_USE_COMPILER% equ 0 (
    echo No parameters specified, using defaults.
    goto mingw_build
)

if %UD_BLD_FLG_USE_COMPILER% equ %UD_BLD_FLG_USE_MINGW%   goto mingw_build
if %UD_BLD_FLG_USE_COMPILER% equ %UD_BLD_FLG_USE_MINGW64% goto mingw_x64_build
if %UD_BLD_FLG_USE_COMPILER% equ %UD_BLD_FLG_USE_WINSDK%  goto winsdk_build

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
    
    :: remove perplexing manifests
    del /S /Q .\bin\*.manifest
    
    :: get rid of annoying dark green color
    color
    
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

    if %WindowsSDKVersionOverride%x neq v7.1x goto NoWin7SDK
    if x%CommandPromptType% neq xCross goto NoWin7SDK
    set path=%PATH%;%VS100COMNTOOLS%\..\..\VC\Bin

    :NoWin7SDK
    rem rebuild modules
    set UD_BUILD_TOOL=lua ..\..\tools\mkmod.lua
    set WXWIDGETS_INC_PATH=%WXWIDGETSDIR%\include
    set WX_CONFIG=%BUILD_ENV%-%1
    if %BUILD_ENV% equ winsdk if %1 equ X86 (
        set WXWIDGETS_LIB_PATH=%WXWIDGETSDIR%\lib\vc_lib%WX_CONFIG%
    )
    if %BUILD_ENV% equ winsdk if %1 equ amd64 (
        set WXWIDGETS_LIB_PATH=%WXWIDGETSDIR%\lib\vc_amd64_lib%WX_CONFIG%
    )
    if %BUILD_ENV% equ winsdk if %1 equ ia64 (
        set WXWIDGETS_LIB_PATH=%WXWIDGETSDIR%\lib\vc_ia64_lib%WX_CONFIG%
    )
    if %BUILD_ENV% equ mingw (
        set WXWIDGETS_LIB_PATH=%WXWIDGETSDIR%\lib\gcc_lib%WX_CONFIG%
    )
    if %BUILD_ENV% equ mingw_x64 (
        set WXWIDGETS_LIB_PATH=%WXWIDGETSDIR%\lib\gcc_lib%WX_CONFIG%
    )
    set WXWIDGETS_INC2_PATH=%WXWIDGETS_LIB_PATH%\mswu
    
    pushd obj\zenwinx
    %UD_BUILD_TOOL% zenwinx.build || goto fail
    cd ..\udefrag
    %UD_BUILD_TOOL% udefrag.build || goto fail
    echo Compile monolithic native interface...
    cd ..\native
    %UD_BUILD_TOOL% defrag_native.build || goto fail
    cd ..\lua5.1
    %UD_BUILD_TOOL% lua.build || goto fail
    cd ..\lua
    %UD_BUILD_TOOL% lua.build || goto fail
    cd ..\lua-gui
    %UD_BUILD_TOOL% lua-gui.build || goto fail
    cd ..\wgx
    %UD_BUILD_TOOL% wgx.build ||  goto fail
    echo Compile other modules...
    cd ..\bootexctrl
    %UD_BUILD_TOOL% bootexctrl.build || goto fail
    cd ..\hibernate
    %UD_BUILD_TOOL% hibernate.build || goto fail
    cd ..\console
    %UD_BUILD_TOOL% defrag.build || goto fail

    cd ..\wxgui
    %UD_BUILD_TOOL% ultradefrag.build || goto fail

    cd ..\gui
    %UD_BUILD_TOOL% ultradefrag.build && goto success

    :fail
    set UD_BUILD_TOOL=
    set WX_CONFIG=
    set WXWIDGETS_INC_PATH=
    set WXWIDGETS_INC2_PATH=
    set WXWIDGETS_LIB_PATH=
    popd
    exit /B 1
    
    :success
    set UD_BUILD_TOOL=
    set WX_CONFIG=
    set WXWIDGETS_INC_PATH=
    set WXWIDGETS_INC2_PATH=
    set WXWIDGETS_LIB_PATH=
    popd

exit /B 0
