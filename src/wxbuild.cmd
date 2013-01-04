@echo off
::
:: Build script for wxWidgets library.
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

call ParseCommandLine.cmd %*

:: handle help request
if %UD_BLD_FLG_DIPLAY_HELP% equ 1 (
    call :usage
    exit /B 0
)

title Build started

:: set environment
call setvars.cmd
if exist "setvars_%COMPUTERNAME%_%ORIG_USERNAME%.cmd" call "setvars_%COMPUTERNAME%_%ORIG_USERNAME%.cmd"
if exist "setvars_%COMPUTERNAME%_%USERNAME%.cmd" call "setvars_%COMPUTERNAME%_%USERNAME%.cmd"

:: configure wxWidgets
copy /Y .\include\wxsetup.h "%WXWIDGETSDIR%\include\wx\msw\setup.h"

:: build wxWidgets
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
        call :build_library X86 || goto fail
    )
    
    set path=%OLD_PATH%

    if %UD_BLD_FLG_BUILD_AMD64% neq 0 (
        echo --------- Target is x64 ---------
        set IA64=
        set AMD64=1
        pushd ..
        call "%WINSDKBASE%\bin\SetEnv.Cmd" /Release /x64 /xp
        popd
        call :build_library amd64 || goto fail
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
        call :build_library ia64 || goto fail
    )
    
    :: get rid of annoying dark green color
    color
    
    goto success

:mingw_x64_build

    set OLD_PATH=%path%

    echo --------- Target is x64 ---------
    set AMD64=1
    set path=%MINGWx64BASE%\bin;%path%
    set BUILD_ENV=mingw_x64
    call :build_library amd64 || goto fail

    goto success

:mingw_build

    set OLD_PATH=%path%

    set path=%MINGWBASE%\bin;%path%
    set BUILD_ENV=mingw
    call :build_library X86 || goto fail

    goto success

:success
if "%OLD_PATH%" neq "" set path=%OLD_PATH%
set OLD_PATH=
echo.
echo Build success!
title Build success!
exit /B 0

:fail
if "%OLD_PATH%" neq "" set path=%OLD_PATH%
set OLD_PATH=
echo Build error (code %ERRORLEVEL%)!
title Build error (code %ERRORLEVEL%)!
exit /B 1

:: Builds wxWidgets library
:: Example: call :build_library X86
:build_library
    set WX_CONFIG=%BUILD_ENV%-%1

    echo.
    echo wxWidgets compilation started...
    echo.
    pushd "%WXWIDGETSDIR%\build\msw"

    if %WindowsSDKVersionOverride%x neq v7.1x goto NoWin7SDK
    if x%CommandPromptType% neq xCross goto NoWin7SDK
    set path=%PATH%;%VS100COMNTOOLS%\..\..\VC\Bin

    :NoWin7SDK
    if %UD_BLD_FLG_ONLY_CLEANUP% equ 1 goto cleanup
    
    set WX_SDK_LIBPATH=%WXWIDGETSDIR%\lib\vc_lib%WX_CONFIG%
    if %1 equ amd64 set WX_SDK_LIBPATH=%WXWIDGETSDIR%\lib\vc_amd64_lib%WX_CONFIG%
    if %1 equ ia64 set WX_SDK_LIBPATH=%WXWIDGETSDIR%\lib\vc_ia64_lib%WX_CONFIG%
    
    :: ia64 targeting SDK compiler doesn't support optimization
    if %1 equ ia64 set WX_SDK_CPPFLAGS=/Od
    
    if %BUILD_ENV% equ winsdk (
        copy /Y "%WXWIDGETSDIR%\include\wx\msw\setup.h" "%WX_SDK_LIBPATH%\mswu\wx\"
        nmake -f makefile.vc TARGET_CPU=%1 BUILD=release UNICODE=1 CFG=%WX_CONFIG% CPPFLAGS="%WX_SDK_CPPFLAGS% /MT" || goto build_failure
    )
    if %BUILD_ENV% equ mingw (
        copy /Y "%WXWIDGETSDIR%\include\wx\msw\setup.h" "%WXWIDGETSDIR%\lib\gcc_lib%WX_CONFIG%\mswu\wx\"
        mingw32-make -f makefile.gcc BUILD=release UNICODE=1 CFG=%WX_CONFIG% CPPFLAGS=-g0 || goto build_failure
    )
    if %BUILD_ENV% equ mingw_x64 (
        copy /Y "%WXWIDGETSDIR%\include\wx\msw\setup.h" "%WXWIDGETSDIR%\lib\gcc_lib%WX_CONFIG%\mswu\wx\"
        mingw32-make -f makefile.gcc BUILD=release UNICODE=1 CFG=%WX_CONFIG% CPPFLAGS="-g0 -m64" || goto build_failure
    )
    goto success
    
    :cleanup
    if %BUILD_ENV% equ winsdk (
        nmake -f makefile.vc TARGET_CPU=%1 BUILD=release UNICODE=1 CFG=%WX_CONFIG% clean || goto build_failure
    )
    if %BUILD_ENV% equ mingw (
        mingw32-make -f makefile.gcc BUILD=release UNICODE=1 CFG=%WX_CONFIG% clean || goto build_failure
    )
    if %BUILD_ENV% equ mingw_x64 (
        mingw32-make -f makefile.gcc BUILD=release UNICODE=1 CFG=%WX_CONFIG% clean || goto build_failure
    )
    goto success
    
    :build_failure
    popd
    set WX_CONFIG=
    set WX_SDK_CPPFLAGS=
    set WX_SDK_LIBPATH=
    exit /B 1
    
    :success
    popd
    echo.
    echo wxWidgets compilation completed successfully!

    set WX_CONFIG=
    set WX_SDK_CPPFLAGS=
    set WX_SDK_LIBPATH=
exit /B 0

rem Displays usage information.
:usage
    echo Usage: wxbuild [options]
    echo.
    echo Common options:
    echo --clean         perform full cleanup instead of the build
    echo.
    echo Compiler:
    echo --use-mingw     (default)
    echo --use-winsdk    (we use it for official releases)
    echo --use-mingw-x64 (experimental, produces wrong x64 code)
    echo.
    echo Target architecture (must always be after compiler):
    echo --no-x86        skip build of 32-bit binaries
    echo --no-amd64      skip build of x64 binaries
    echo --no-ia64       skip build of IA-64 binaries
    echo.
    echo Example:
    echo wxbuild --use-winsdk --no-amd64 --no-ia64 --clean
    echo.
    echo In this case the x86 winsdk build will be cleaned up.
    echo.
    echo Without parameters the wxbuild command uses MinGW
    echo to compile 32-bit libraries.
goto :EOF
