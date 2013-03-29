@echo off
::
:: Script to upload translations to the translation project at
:: https://www.transifex.com/projects/p/ultradefrag/
:: Copyright (c) 2013 Stefan Pendl (stefanpe@users.sourceforge.net).
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

title Upload started

:: set environment
call setvars.cmd
if exist "setvars_%COMPUTERNAME%_%ORIG_USERNAME%.cmd" call "setvars_%COMPUTERNAME%_%ORIG_USERNAME%.cmd"
if exist "setvars_%COMPUTERNAME%_%USERNAME%.cmd" call "setvars_%COMPUTERNAME%_%USERNAME%.cmd"
echo.

:: upload all translations to transifex
pushd "%~dp0\tools\transifex"
if exist tx.exe tx.exe push -t --skip || popd && goto fail
echo.
popd

:success
echo.
echo Translations uploaded sucessfully!
title Translations uploaded sucessfully!
popd
exit /B 0

:fail
echo Build error (code %ERRORLEVEL%)!
title Build error (code %ERRORLEVEL%)!
popd
exit /B 1
