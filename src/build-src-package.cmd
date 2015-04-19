@echo off
::
:: This script builds UltraDefrag source code package.
:: Copyright (c) 2007-2015 Dmitri Arkhangelski (dmitriar@gmail.com).
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

:: set environment variables
if "%ULTRADFGVER%" == "" (
    call setvars.cmd
    if exist "setvars_%COMPUTERNAME%_%ORIG_USERNAME%.cmd"^
        call "setvars_%COMPUTERNAME%_%ORIG_USERNAME%.cmd"
    if exist "setvars_%COMPUTERNAME%_%USERNAME%.cmd"^
        call "setvars_%COMPUTERNAME%_%USERNAME%.cmd"
)

pushd ..
"%SEVENZIP_PATH%\7z.exe" a -r -mx9 -x@src\exclude-from-sources.lst^
    ultradefrag-%UDVERSION_SUFFIX%.src.7z doc src || goto fail
popd

if not exist release mkdir release
move /Y ..\ultradefrag-%UDVERSION_SUFFIX%.src.7z .\release\ || exit /B 1
exit /B 0

:fail
popd
exit /B 1
