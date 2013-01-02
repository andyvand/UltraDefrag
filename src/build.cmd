@echo off
::
:: Build script for the UltraDefrag project.
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

if %UD_BLD_FLG_ONLY_CLEANUP% equ 1 (
    call :cleanup
    exit /B 0
)

title Build started

:: set environment
call :set_build_environment

:: build all binaries
call build-targets.cmd %* || goto fail

:: build documentation
call build-docs.cmd %* || goto fail

:: update installer lang.ini
call make-lang-ini.cmd || goto fail

:: build installers and portable packages
echo Build installers and/or portable packages...
if %UD_BLD_FLG_BUILD_X86% neq 0 (
    call :build_installer              .\bin i386 || goto fail
    call :build_portable_package       .\bin i386 || goto fail
)
if %UD_BLD_FLG_BUILD_AMD64% neq 0 (
    call :build_installer              .\bin\amd64 amd64 || goto fail
    call :build_portable_package       .\bin\amd64 amd64 || goto fail
)
if %UD_BLD_FLG_BUILD_IA64% neq 0 (
    call :build_installer              .\bin\ia64 ia64 || goto fail
    call :build_portable_package       .\bin\ia64 ia64 || goto fail
)
echo.
echo Build success!
title Build success!

:: install the program if requested
if %UD_BLD_FLG_DO_INSTALL% neq 0 (
    call :install || exit /B 1
)

:: all operations successfully completed!
exit /B 0

:fail
echo Build error (code %ERRORLEVEL%)!
title Build error (code %ERRORLEVEL%)!
exit /B 1

:: --------------------------------------------------------
::                      subroutines
:: --------------------------------------------------------

rem Sets environment for the build process.
:set_build_environment
    call setvars.cmd
    if exist "setvars_%COMPUTERNAME%_%ORIG_USERNAME%.cmd" call "setvars_%COMPUTERNAME%_%ORIG_USERNAME%.cmd"
    if exist "setvars_%COMPUTERNAME%_%USERNAME%.cmd" call "setvars_%COMPUTERNAME%_%USERNAME%.cmd"

    if %UD_BLD_FLG_IS_PORTABLE% equ 1 (
        set UDEFRAG_PORTABLE=1
    ) else (
        set UDEFRAG_PORTABLE=
    )

    if %UD_BLD_FLG_BUILD_ALL% equ 1 (
        set UDEFRAG_PORTABLE=
    )

    echo #define VERSION %VERSION% > .\include\ultradfgver.new
    echo #define VERSION2 %VERSION2% >> .\include\ultradfgver.new
    if "%RELEASE_STAGE%" neq "" (
        echo #define VERSIONINTITLE "UltraDefrag %ULTRADFGVER% %RELEASE_STAGE%" >> .\include\ultradfgver.new
        echo #define VERSIONINTITLE_PORTABLE "UltraDefrag %ULTRADFGVER% %RELEASE_STAGE% Portable" >> .\include\ultradfgver.new
        echo #define ABOUT_VERSION "Ultra Defragmenter version %ULTRADFGVER% %RELEASE_STAGE%" >> .\include\ultradfgver.new
    ) else (
        echo #define VERSIONINTITLE "UltraDefrag %ULTRADFGVER%" >> .\include\ultradfgver.new
        echo #define VERSIONINTITLE_PORTABLE "UltraDefrag %ULTRADFGVER% Portable" >> .\include\ultradfgver.new
        echo #define ABOUT_VERSION "Ultra Defragmenter version %ULTRADFGVER%" >> .\include\ultradfgver.new
    )
    
    rem remove preview menu for release candidates
    if /I "%RELEASE_STAGE:~0,2%" equ "RC" echo #define _UD_HIDE_PREVIEW_ >> .\include\ultradfgver.new
    rem remove preview menu for final release
    if "%RELEASE_STAGE%" equ "" echo #define _UD_HIDE_PREVIEW_ >> .\include\ultradfgver.new
    
    rem update ultradfgver.h only when something
    rem changed to speed incremental compilation up
    fc .\include\ultradfgver.new .\include\ultradfgver.h >nul
    if errorlevel 1 move /Y .\include\ultradfgver.new .\include\ultradfgver.h
    del /q .\include\ultradfgver.new

    rem force zenwinx version to be the same as ultradefrag version
    echo #define ZENWINX_VERSION %VERSION% > .\dll\zenwinx\zenwinxver.new
    echo #define ZENWINX_VERSION2 %VERSION2% >> .\dll\zenwinx\zenwinxver.new

    rem update zenwinxver.h only when something
    rem changed to speed incremental compilation up
    fc .\dll\zenwinx\zenwinxver.new .\dll\zenwinx\zenwinxver.h >nul
    if errorlevel 1 move /Y .\dll\zenwinx\zenwinxver.new .\dll\zenwinx\zenwinxver.h
    del /q .\dll\zenwinx\zenwinxver.new
goto :EOF

rem Synopsis: call :build_installer {path to binaries} {arch}
rem Example:  call :build_installer .\bin\ia64 ia64
:build_installer
    if %UD_BLD_FLG_BUILD_ALL% neq 1 if "%UDEFRAG_PORTABLE%" neq "" exit /B 0

    pushd %1
    copy /Y "%~dp0\installer\UltraDefrag.nsi" .\
    copy /Y "%~dp0\installer\lang.ini" .\

    echo The Ultra Defragmenter Version %ULTRADFGVER% >.\README.TXT
    type "%~dp0\README.TXT" >>.\README.TXT

    if "%RELEASE_STAGE%" neq "" (
        set NSIS_COMPILER_FLAGS=/DULTRADFGVER=%ULTRADFGVER% /DULTRADFGARCH=%2 /DRELEASE_STAGE=%RELEASE_STAGE% /DUDVERSION_SUFFIX=%UDVERSION_SUFFIX%
    ) else (
        set NSIS_COMPILER_FLAGS=/DULTRADFGVER=%ULTRADFGVER% /DULTRADFGARCH=%2 /DUDVERSION_SUFFIX=%UDVERSION_SUFFIX%
    )
    "%NSISDIR%\makensis.exe" %NSIS_COMPILER_FLAGS% UltraDefrag.nsi
    if %errorlevel% neq 0 (
        set NSIS_COMPILER_FLAGS=
        popd
        exit /B 1
    )
    set NSIS_COMPILER_FLAGS=
    popd
exit /B 0

rem Synopsis: call :build_portable_package {path to binaries} {arch}
rem Example:  call :build_portable_package .\bin\ia64 ia64
:build_portable_package
    if %UD_BLD_FLG_BUILD_ALL% neq 1 if "%UDEFRAG_PORTABLE%" equ "" exit /B 0
    
    pushd %1
    set PORTABLE_DIR=ultradefrag-portable-%UDVERSION_SUFFIX%.%2
    mkdir %PORTABLE_DIR%
    copy /Y "%~dp0\CREDITS.TXT" %PORTABLE_DIR%\
    copy /Y "%~dp0\HISTORY.TXT" %PORTABLE_DIR%\
    copy /Y "%~dp0\LICENSE.TXT" %PORTABLE_DIR%\
    
    echo The Ultra Defragmenter Version %ULTRADFGVER% >%PORTABLE_DIR%\README.TXT
    type "%~dp0\README.TXT" >>%PORTABLE_DIR%\README.TXT
    
    copy /Y hibernate.exe       %PORTABLE_DIR%\hibernate4win.exe
    copy /Y udefrag.dll         %PORTABLE_DIR%\
    copy /Y udefrag.exe         %PORTABLE_DIR%\
    copy /Y zenwinx.dll         %PORTABLE_DIR%\
    copy /Y ultradefrag.exe     %PORTABLE_DIR%
    copy /Y lua5.1a.dll         %PORTABLE_DIR%\
    copy /Y lua5.1a.exe         %PORTABLE_DIR%\
    copy /Y lua5.1a_gui.exe     %PORTABLE_DIR%\
    copy /Y wgx.dll             %PORTABLE_DIR%\
    mkdir %PORTABLE_DIR%\handbook
    copy /Y "%~dp0\..\doc\handbook\doxy-doc\html\*.*" %PORTABLE_DIR%\handbook\
    mkdir %PORTABLE_DIR%\scripts
    copy /Y "%~dp0\scripts\udreportcnv.lua"      %PORTABLE_DIR%\scripts\
    copy /Y "%~dp0\scripts\udsorting.js"         %PORTABLE_DIR%\scripts\
    copy /Y "%~dp0\scripts\udreport.css"         %PORTABLE_DIR%\scripts\
    copy /Y "%~dp0\scripts\upgrade-guiopts.lua"  %PORTABLE_DIR%\scripts\
    copy /Y "%~dp0\scripts\upgrade-rptopts.lua"  %PORTABLE_DIR%\scripts\
    mkdir %PORTABLE_DIR%\options
    lua "%~dp0\scripts\upgrade-guiopts.lua"  %PORTABLE_DIR%
    lua "%~dp0\scripts\upgrade-rptopts.lua"  %PORTABLE_DIR%
    mkdir %PORTABLE_DIR%\i18n
    copy /Y "%~dp0\gui\i18n\*.lng"           %PORTABLE_DIR%\i18n\
    copy /Y "%~dp0\gui\i18n\*.template"      %PORTABLE_DIR%\i18n\

    "%SEVENZIP_PATH%\7z.exe" a -r -mx9 -tzip ultradefrag-portable-%UDVERSION_SUFFIX%.bin.%2.zip %PORTABLE_DIR%
    if %errorlevel% neq 0 (
        rd /s /q %PORTABLE_DIR%
        set PORTABLE_DIR=
        popd
        exit /B 1
    )
    rd /s /q %PORTABLE_DIR%
    set PORTABLE_DIR=
    popd
exit /B 0

rem Installs the program.
:install
    if "%UDEFRAG_PORTABLE%" neq "" exit /B 0

    setlocal
    if /i %PROCESSOR_ARCHITECTURE% == x86 (
        set INSTALLER_PATH=.\bin
        set INSTALLER_ARCH=i386
    )
    if /i %PROCESSOR_ARCHITECTURE% == AMD64 (
        set INSTALLER_PATH=.\bin\amd64
        set INSTALLER_ARCH=amd64
    )
    if /i %PROCESSOR_ARCHITECTURE% == IA64 (
        set INSTALLER_PATH=.\bin\ia64
        set INSTALLER_ARCH=ia64
    )
    set INSTALLER_NAME=ultradefrag

    echo Start installer...
    %INSTALLER_PATH%\%INSTALLER_NAME%-%UDVERSION_SUFFIX%.bin.%INSTALLER_ARCH%.exe /S
    if %errorlevel% neq 0 (
        echo Install error!
        endlocal
        exit /B 1
    )
    echo Install success!
    endlocal
exit /B 0

rem Cleans up sources directory
rem by removing all intermediate files.
:cleanup
    echo Delete all intermediate files...
    rd /s /q bin
    rd /s /q lib
    rd /s /q obj
    rd /s /q doxy-doc
    rd /s /q gui\doxy-doc
    rd /s /q dll\wgx\doxy-doc
    rd /s /q dll\udefrag\doxy-doc
    rd /s /q dll\zenwinx\doxy-doc
    rd /s /q ..\doc\handbook\doxy-doc
    rd /s /q doxy-defaults
    rd /s /q gui\doxy-defaults
    rd /s /q dll\wgx\doxy-defaults
    rd /s /q dll\udefrag\doxy-defaults
    rd /s /q dll\zenwinx\doxy-defaults
    rd /s /q ..\doc\handbook\doxy-defaults
    rd /s /q ..\doc\handbook\doxy-defaults_a4
    rd /s /q ..\doc\handbook\doxy-defaults_letter
    rd /s /q src_package
    rd /s /q ..\src_package
    if %UD_BLD_FLG_ONLY_CLEANUP% equ 1 rd /s /q release

    del /f /q ..\..\web\doxy-doc\html\*.*
    del /f /q ..\..\web\doxy-doc\udefrag.dll\html\*.*
    del /f /q ..\..\web\doxy-doc\wgx\html\*.*

    for %%F in ( "..\..\web\handbook\*.*" ) do if not "%%~nxF" == ".htaccess" del /f /q "%%~F"

    echo Done.
goto :EOF

rem Displays usage information.
:usage
    echo Usage: build [options]
    echo.
    echo Common options:
    echo --portable      build portable packages instead of installers
    echo --all           build all packages: regular and portable
    echo --install       perform silent installation after the build
    echo --clean         perform full cleanup instead of the build
    echo --no-pdf        skip building of PDF documentation
    echo --no-dev        skip building of development documentation
    echo --internal-doc  build only internal development documentation
    echo.
    echo Compiler:
    echo --use-mingw     (default)
    echo --use-winsdk    (we use it for official 64-bit releases)
    echo --use-mingw-x64 (experimental, produces wrong x64 code)
    echo.
    echo Target architecture (must always be after compiler):
    echo --no-x86        skip build of 32-bit binaries
    echo --no-amd64      skip build of x64 binaries
    echo --no-ia64       skip build of IA-64 binaries
    echo.
    echo Without parameters the build command uses MinGW to build
    echo a 32-bit regular installer.
    echo.
    echo * To use MinGW run mingw_patch.cmd before:
    echo dll\zenwinx\mingw_patch.cmd {path to mingw installation}
    echo.
    echo * To use Windows SDK run winsdk_patch.cmd before:
    echo dll\zenwinx\winsdk_patch.cmd {path to sdk installation}
goto :EOF
