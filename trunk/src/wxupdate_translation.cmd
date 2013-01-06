@echo off
::
:: Script to update the translations using GNU gettext.
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

if not exist "%GNUWIN32_DIR%" goto gnuwin32_missing

:: configure PATH
set OLD_PATH=%PATH%
set PATH=%GNUWIN32_DIR%;%PATH%

pushd "%~dp0\wxgui"

set gettext_template=.\locale\UltraDefrag_template.po

:: extract translations
xgettext -C -n -j -k_ -kwxPLURAL:1,2 -kwxTRANSLATE -o %gettext_template% ".\*.cpp" || goto fail

:: update translations
for /d %%D in ( ".\locale\*" ) do for %%T in ( "%%~D\UltraDefrag.po" ) do call :check_translation "%%~T" || goto fail

:success
if "%OLD_PATH%" neq "" set path=%OLD_PATH%
set OLD_PATH=
echo.
echo Build success!
title Build success!
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
    echo processing file "%~1" ...

    rem make sure the translation file is present
    echo     checking for files existance ...
    if not exist "%~1" copy /y /v %gettext_template% "%~1" || goto check_fail

    rem merge the new strings into the translation file if necessary
    echo     merging in new strings ...
    msgmerge -U "%~1" %gettext_template% || goto check_fail

    rem compile the .PO file into a .MO file
    echo     compiling ...
    msgfmt "%~1" -o "%~dpn1.mo" || goto check_fail

    :check_success
    echo suceeded ...
    echo ---------------
    if exist "%~1~" del /f /q "%~1~"
    exit /B 0

    :check_fail
    failed ...
    echo ---------------
    exit /B 1
goto :EOF
