@echo off
::
:: Script to update the translations using GNU gettext and the
:: translation project at https://www.transifex.com/projects/p/ultradefrag/
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

title Update started

:: set environment
call setvars.cmd
if exist "setvars_%COMPUTERNAME%_%ORIG_USERNAME%.cmd" call "setvars_%COMPUTERNAME%_%ORIG_USERNAME%.cmd"
if exist "setvars_%COMPUTERNAME%_%USERNAME%.cmd" call "setvars_%COMPUTERNAME%_%USERNAME%.cmd"
echo.

if not exist "%GNUWIN32_DIR%" goto gnuwin32_missing

:: configure PATH
set OLD_PATH=%PATH%
set PATH=%GNUWIN32_DIR%;%PATH%

pushd "%~dp0\wxgui"

:: extract translations
xgettext -C -j --copyright-holder="UltraDefrag Development Team" --msgid-bugs-address="http://sourceforge.net/p/ultradefrag/bugs/" -k_ -kwxPLURAL:1,2 -kwxTRANSLATE -kUD_UpdateMenuItemLabel:2 -o locale\UltraDefrag.pot *.cpp || goto fail
copy /v /y locale\UltraDefrag.pot "%~dp0\tools\transifex\translations\ultradefrag.main\en_US.po"
echo.

:: download all translations from transifex
pushd "%~dp0\tools\transifex"
if exist tx.exe tx.exe pull -a || (popd && goto fail)
echo.
popd

:: update translations
for %%F in ( "%~dp0\tools\transifex\translations\ultradefrag.main\*.po" ) do call :check_translation "%%~F" || goto fail

:success
if "%OLD_PATH%" neq "" set path=%OLD_PATH%
set OLD_PATH=
echo.
echo Translations updated sucessfully!
title Translations updated sucessfully!
popd
exit /B 0

:gnuwin32_missing
echo !!! GNUwin32 path not set correctly !!!
set ERRORLEVEL=99

:fail
if "%OLD_PATH%" neq "" set path=%OLD_PATH%
set OLD_PATH=
echo Build error (code %ERRORLEVEL%)!
title Build error (code %ERRORLEVEL%)!
popd
exit /B 1

:: syntax: call :check_translation {.PO file}
:check_translation
    echo.
    echo ---------------
    echo processing locale "%~n1"

    rem make sure the translation folder is present
    echo ... checking for folders existance
    if not exist "locale\%~n1" mkdir "locale\%~n1" || goto check_fail

    rem compile the .PO file into a .MO file
    echo ... compiling
    msgfmt -v -o "locale\%~n1\UltraDefrag.mo" "%~1" || goto check_fail

    :check_success
    echo ... suceeded
    exit /B 0

    :check_fail
    echo ... failed
    exit /B 1
goto :EOF
