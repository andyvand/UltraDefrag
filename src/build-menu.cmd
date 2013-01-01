@echo off

::
:: This script is a simple front-end for the build commands;
:: it includes the most common build situations.
:: Copyright (c) 2010-2012 Stefan Pendl (stefanpe@users.sourceforge.net).
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

set UD_BLD_MENU_DIR=%~dp0

:DisplayMenu
title UltraDefrag Project - Build Menu
cls
echo.
echo UltraDefrag Project - Build Menu
echo ================================
echo.
echo   Brought to You by the UltraDefrag Development Team
echo.
echo      1 ... Clean Project Folder
echo      2 ... Build with Defaults
echo      3 ... Build with Defaults and Install
echo.
echo      4 ... Build Release
echo.
echo      5 ... Build .................. using WinSDK, no IA64
echo      6 ... Build Portable ......... using WinSDK, no IA64
echo.
echo      7 ... Build Docs
echo      8 ... Build only Handbook
echo      9 ... Build internal docs
echo.
echo     10 ... Build .................. with Custom Switches
echo     11 ... Build Portable ......... with Custom Switches
echo.
echo     12 ... Clean all wxWidgets libraries
echo     13 ... Build wxWidgets ........ using MinGW
echo     14 ... Build wxWidgets ........ using WinSDK
echo.
echo     15 ... Build Test Release for Stefan
echo     16 ... Build Test Installation for Stefan
echo     17 ... Build Test AMD64 for Stefan
echo     18 ... Build Test x86 for Stefan
echo.
echo      0 ... EXIT

:: this value holds the number of the last menu entry
set UD_BLD_MENU_MAX_ENTRIES=18

:AskSelection
echo.
set /p UD_BLD_MENU_SELECT="Please select an Option: "
echo.

if %UD_BLD_MENU_SELECT% LEQ %UD_BLD_MENU_MAX_ENTRIES% goto %UD_BLD_MENU_SELECT%

color 8C
echo Wrong Selection !!!
color
set UD_BLD_MENU_SELECT=
goto AskSelection

:0
goto :EOF

:1
title Clean Project Folder
call build.cmd --clean
goto finished

:2
title Build with Defaults
call build.cmd
goto finished

:3
title Build with Defaults and Install
call build.cmd --install --no-pdf --no-dev
goto finished

:4
title Build Release
call build-release.cmd
goto finished

:5
title Build .................. using WinSDK, no IA64
call build.cmd --use-winsdk --no-ia64
goto finished

:6
title Build Portable ......... using WinSDK, no IA64
call build.cmd --portable --use-winsdk --no-ia64
goto finished

:7
title Build Docs
call build.cmd --clean
call build-docs.cmd
goto finished

:8
title Build only Handbook
call build.cmd --clean
call build-docs.cmd --no-dev
goto finished

:9
title Build internal docs
call build.cmd --clean
call build-docs.cmd --internal-doc
goto finished

:10
title Build .................. with Custom Switches
cls
echo.
call build.cmd --help
echo.
set /p UD_BLD_MENU_SWITCH="Please enter Switches: "

title Build .................. %UD_BLD_MENU_SWITCH%
echo.
call build.cmd %UD_BLD_MENU_SWITCH%
goto finished

:11
title Build Portable ......... with Custom Switches
cls
echo.
call build.cmd --help
echo.
set /p UD_BLD_MENU_SWITCH="Please enter Switches: "

title Build Portable ......... %UD_BLD_MENU_SWITCH%
echo.
call build.cmd --portable %UD_BLD_MENU_SWITCH%
goto finished

:12
title Clean all wxWidgets libraries
call wxbuild.cmd --use-mingw --clean
call wxbuild.cmd --use-winsdk --clean
goto finished

:13
title Build wxWidgets ........ using MinGW
call wxbuild.cmd --use-mingw
goto finished

:14
title Build wxWidgets ........ using WinSDK
call wxbuild.cmd --use-winsdk
goto finished

:15
title Build Test Release for Stefan
echo.
call build.cmd --no-amd64 --no-ia64 --no-pdf --no-dev
call build.cmd --use-winsdk --no-x86 --no-ia64 --no-pdf --no-dev
echo.
call :CopyInstallers -zip
goto finished

:16
title Build Test Installation for Stefan
echo.
if %PROCESSOR_ARCHITECTURE% == AMD64 call build.cmd --use-winsdk --no-ia64 --no-x86 --install --no-pdf --no-dev
if %PROCESSOR_ARCHITECTURE% == x86 call build.cmd --no-ia64 --no-amd64 --install --no-pdf --no-dev
echo.
goto finished

:17
title Build Test AMD64 for Stefan
echo.
call build.cmd --use-winsdk --no-ia64 --no-x86 --no-pdf --no-dev
echo.
call :CopyInstallers
goto finished

:18
title Build Test x86 for Stefan
echo.
call build.cmd --no-ia64 --no-amd64 --no-pdf --no-dev
echo.
call :CopyInstallers
goto finished

:finished
echo.
pause

goto :EOF

:CopyInstallers
if not exist "%USERPROFILE%\Downloads\UltraDefrag" mkdir "%USERPROFILE%\Downloads\UltraDefrag"
cd /d "%USERPROFILE%\Downloads\UltraDefrag"
echo.
copy /b /y /v "%UD_BLD_MENU_DIR%\bin\ultradefrag-%UDVERSION_SUFFIX%.bin.i386.exe"        .
copy /b /y /v "%UD_BLD_MENU_DIR%\bin\amd64\ultradefrag-%UDVERSION_SUFFIX%.bin.amd64.exe" .
echo.
if "%~1" == "" goto :noZip
del /f /q "ultradefrag-%UDVERSION_SUFFIX%.7z"
"%SEVENZIP_PATH%\7z.exe" a -t7z -mx9 -pud -mhe=on "ultradefrag-%UDVERSION_SUFFIX%.7z" "ultradefrag-%UDVERSION_SUFFIX%.bin.*.exe"
:noZip
echo.
goto :EOF