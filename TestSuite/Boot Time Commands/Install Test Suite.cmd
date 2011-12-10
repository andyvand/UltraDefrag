@echo off

::
:: This script will install the Ultradefrag Boot Time Defrag Test Suite on your system.
:: Copyright (c) 2010 by Stefan Pendl (stefanpe@users.sourceforge.net).
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

:: check for administrative rights
for /f "tokens=2 delims=[" %%V in ('ver') do set test=%%V
for /f "tokens=2" %%V in ('echo %test%') do set test1=%%V
for /f "tokens=1,2 delims=." %%V in ('echo %test1%') do set OSversion=%%V.%%W

if %OSversion% LSS 6.0 goto :IsAdmin

if /i %USERNAME% == Administrator goto :IsAdmin
if not defined SESSIONNAME goto :IsAdmin

echo.
echo This script must be run by the Administrator !!!
goto :quit

:IsAdmin
cd /D %~dp0

rem For Windows XP x64 Edition we need to force notepad.
rem For Windows Vista and higher x64 Editions
rem there is the SysNative workaround for Notepad++
set force_default_editor=0

set default_editor=notepad
set boot_time_scripts=ud-boot-time.cmd "%SystemRoot%\System32\ud-boot-time.cmd"

for /F "tokens=2 delims=[]" %%V in ('ver') do for /F "tokens=2" %%R in ( 'echo %%V' ) do set win_ver=%%R

if %PROCESSOR_ARCHITECTURE% == AMD64 (
    if %win_ver% LSS 6 (
        set force_default_editor=1
    ) else (
        set boot_time_scripts=ud-boot-time.cmd "%SystemRoot%\SysNative\ud-boot-time.cmd"
    )
)

title Installing Ultradefrag Boot Time Test Suite

echo.
echo Select Action for Default Test Suite Boot Time Script
echo.
echo 1 ... Replace existing Boot Time Script
echo 2 ... Edit    existing Boot Time Script
echo 3 ... Only Copy Test Scripts
echo.
echo 0 ... EXIT
echo.
set /P answer="Select an Option: "
echo.

if %answer% LEQ 3 goto %answer%

:0
goto :quit

:1
copy /b /v /y ud-boot-time.cmd %SystemRoot%\System32
goto :3

:2
set editor_cmd=%default_editor%

if %force_default_editor% == 1 goto :3

set editor_cmd="%ProgramFiles(x86)%\Notepad++\notepad++.exe"
if not exist %editor_cmd% set editor_cmd="%ProgramFiles%\Notepad++\notepad++.exe"
if not exist %editor_cmd% set editor_cmd=%default_editor%

:3
rem copy test command scripts
copy /b /v /y ud-boot-time-test-*.cmd C:\

if %answer% NEQ 2 goto :quit

if %editor_cmd% == %default_editor% (
    for %%F in ( %boot_time_scripts% ) do start "" %editor_cmd% "%%~F"
) else (
    %editor_cmd% %boot_time_scripts%
)

:quit
echo.
pause
