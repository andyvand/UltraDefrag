@echo off

::
:: This script patches all the development tools
:: to make 'em suitable for UltraDefrag.
:: Copyright (c) 2007-2013 Dmitri Arkhangelski (dmitriar@gmail.com).
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

call setvars.cmd
if exist "setvars_%COMPUTERNAME%_%ORIG_USERNAME%.cmd" call "setvars_%COMPUTERNAME%_%ORIG_USERNAME%.cmd"
if exist "setvars_%COMPUTERNAME%_%USERNAME%.cmd" call "setvars_%COMPUTERNAME%_%USERNAME%.cmd"

if "%WXWIDGETSDIR%" neq "" (
    echo.
    echo Patching wxWidgets
    echo ------------------
    echo.
    copy /Y tools\patch\wx\intl.cpp "%WXWIDGETSDIR%\src\common\" || goto fail
)

if "%WINSDKBASE%" neq "" (
    echo.
    echo Patching Windows SDK
    echo --------------------
    echo.
    copy /Y tools\patch\winsdk\ntdll.lib "%WINSDKBASE%\Lib\" || goto fail
    copy /Y tools\patch\winsdk\amd64\ntdll.lib "%WINSDKBASE%\Lib\x64" || goto fail
    copy /Y tools\patch\winsdk\ia64\ntdll.lib "%WINSDKBASE%\Lib\IA64" || goto fail
)

if "%MINGWBASE%" neq "" (
    echo.
    echo Patching MinGW
    echo --------------
    echo.
    pushd "%MINGWBASE%\lib"
    if not exist libntdll.a.orig move libntdll.a libntdll.a.orig
    popd
    pushd tools\patch\mingw
    "%MINGWBASE%\bin\dlltool" -k --output-lib libntdll.a --def ntdll.def || goto fail
    move /Y libntdll.a "%MINGWBASE%\lib\libntdll.a" || goto fail
    popd
)

echo.
echo Patching succeeded!
exit /B 0

:fail
echo.
echo Patching failed!
exit /B 1
