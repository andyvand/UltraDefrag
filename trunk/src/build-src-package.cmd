@echo off

::
:: This script builds UltraDefrag source code package.
:: Copyright (c) 2007-2012 by Dmitri Arkhangelski (dmitriar@gmail.com).
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

echo Build source code package...

:: set environment variables if they aren't already set
if "%ULTRADFGVER%" == "" (
    call setvars.cmd
    if exist "setvars_%COMPUTERNAME%_%ORIG_USERNAME%.cmd" call "setvars_%COMPUTERNAME%_%ORIG_USERNAME%.cmd"
    if exist "setvars_%COMPUTERNAME%_%USERNAME%.cmd" call "setvars_%COMPUTERNAME%_%USERNAME%.cmd"
)

:: recreate src_package directory
rmdir /s /q .\src_package
mkdir .\src_package

:: set SRC_PKG_PATH pointing to directory one level above the current directory
set SRC_PKG_PATH=..\src_package\ultradefrag-%UDVERSION_SUFFIX%
rmdir /s /q ..\src_package
mkdir ..\src_package

:: copy all important source files with generated documentation
xcopy /I /Y /Q /S /EXCLUDE:exclude-from-sources.lst . %SRC_PKG_PATH%\src
xcopy /I /Y /Q /S .\doxy-doc              %SRC_PKG_PATH%\src\doxy-doc
xcopy /I /Y /Q /S .\dll\zenwinx\doxy-doc  %SRC_PKG_PATH%\src\dll\zenwinx\doxy-doc
xcopy /I /Y /Q /S ..\doc\html\handbook    %SRC_PKG_PATH%\doc\html\handbook

:: make source code package
cd ..\src_package
REM zip -r -m -9 -X ultradefrag-%ULTRADFGVER%.src.zip .
"%SEVENZIP_PATH%\7z.exe" a -r -mx9 ultradefrag-%UDVERSION_SUFFIX%.src.7z * || exit /B 1
rd /s /q ultradefrag-%UDVERSION_SUFFIX%
cd ..\src
move /Y ..\src_package\ultradefrag-%UDVERSION_SUFFIX%.src.7z .\src_package\
rmdir /s /q ..\src_package
exit /B 0
