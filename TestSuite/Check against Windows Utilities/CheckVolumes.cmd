@rem 
@echo off

::
:: This test utility collects output of Windows utilities chkdsk and defrag.
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
for /F "tokens=2 delims=[]" %%V in ('ver') do for /F "tokens=2" %%R in ( 'echo %%V' ) do set win_ver=%%R

echo.
for /F "tokens=1 skip=8" %%D in ( 'udefrag -l' ) do call :process %%D

:quit
echo.
pause
goto :EOF

:process
    set drive_tmp=%1
    set drive=%drive_tmp:~0,1%
    
    echo Processing %1 ...
    echo      Running CHKDSK ....
    call :RunUtility chkdsk %1 >"%COMPUTERNAME%_%drive%_chkdsk.log" 2>nul
    
    if %win_ver% GEQ 5.1 (
        echo      Running DEFRAG ....
        call :RunUtility defrag %1 >"%COMPUTERNAME%_%drive%_defrag.log" 2>nul
    )
    
    echo      Running UDEFRAG ...
    call :RunUtility udefrag %1 >"%COMPUTERNAME%_%drive%_udefrag.log" 2>nul
    echo ------------------------
goto :EOF

:RunUtility
    echo.
    echo Started "%*" at %DATE% - %TIME%
    echo -------------------------------
    echo.
    if /i "%1" == "chkdsk"  chkdsk %2 | findstr /v /i "Prozent percent"
    if /i "%1" == "defrag"  defrag -a -v %2
    if /i "%1" == "udefrag" udefrag -a -v %2
    echo.
    echo -------------------------------
    echo Finished "%*" at %DATE% - %TIME%
goto :EOF
