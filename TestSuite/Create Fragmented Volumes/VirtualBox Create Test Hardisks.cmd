@rem 
@echo off
::
:: This script creates virtual harddisks
:: It is tested with VirtualBox v4.1
::
:: It can be used to create NewHardDisk{Index}.vdi files
:: in the folders containing virtual machines
::
:: Copyright (c) 2010-2011 by Stefan Pendl (stefanpe@users.sourceforge.net).
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

setlocal enabledelayedexpansion

title %~n0

:: installation folder of VirtualBox
for /f "tokens=2*" %%I in ('reg query "HKLM\SOFTWARE\Oracle\VirtualBox" /v InstallDir') do set VBoxRoot=%%~J
if "%VBoxRoot%" == "" goto :noVBroot
if not exist "%VBoxRoot%" goto :noVBroot

:: Folder containing the virtual machines
if not exist "%USERPROFILE%\.VirtualBox\VirtualBox.xml" goto :noVMRoot

for /f "tokens=2" %%D in ('findstr "defaultMachineFolder" "%USERPROFILE%\.VirtualBox\VirtualBox.xml"') do set VMRootTMP1=%%~D
set VMRootTMP=%VMRootTMP1:"=%
if "%VMRootTMP%" == "" goto :noVMRoot

for /f "tokens=2 delims==" %%D in ('echo "%VMRootTMP%"') do set VMRootTMP1=%%~D
set VMRoot=%VMRootTMP1:"=%
if "%VMRoot%" == "" goto :noVMRoot
if not exist "%VMRoot%" goto :noVMRoot

cd /d %VMRoot%

:DisplayVMlist
cls
set MenuItem=0
set MenuSelectionsTMP=
echo.
for /d %%V in ( * ) do set /a MenuItem+=1 & echo !MenuItem! ... "%%~V" & set MenuSelectionsTMP=!MenuSelectionsTMP!:%%~V

set MenuSelections=%MenuSelectionsTMP:~1%
echo.
echo 0 ... EXIT
echo.
set /p SelectedItem="Select the VM to create disks for: "

if "%SelectedItem%" == "" goto :quit
if %SelectedItem% EQU 0 goto :quit

if %SelectedItem% LEQ %MenuItem% goto :CreateDisks

echo.
echo Please enter a number in the range of 0 to %MenuItem%.
echo.
pause
goto :DisplayVMlist

:CreateDisks
for /f "tokens=%SelectedItem% delims=:" %%S in ('echo %MenuSelections%') do set SelectedVM=%%~S

echo.
echo For Windows 2000 the maximum disk count is 8,
echo since the controller dosn't support more!
echo.
set /p MaxIndex="Enter number of disks to create (0 to exit, maximum 15): "
if "%MaxIndex%" == "" goto :quit
if %MaxIndex% EQU 0 goto :quit
if %MaxIndex% GTR 15 set MaxIndex=15

cd /d %VBoxRoot%

for /L %%F in (1,1,%MaxIndex%) do (
    set HardDiskName="%VMRoot%\%SelectedVM%\%SelectedVM: =_%_TestDisk_%%F.vdi"
    
    echo.
    echo ---------------------------------------
    echo.
    
    if not exist !HardDiskName! (
        echo Creating !HardDiskName! ...
        echo.
        VBoxManage createhd --filename !HardDiskName! --size 1024
    ) else (
        echo Modifying !HardDiskName! ...
        echo.
        VBoxManage modifyhd !HardDiskName! --resize 1024
    )
)

:quit
echo.
echo ---------------------------------------
echo.
pause

goto :EOF

:noVBroot
echo.
echo VirtualBox not installed, aborting ...
goto :quit

:noVMRoot
echo.
echo Harddisk Root Folder missing, aborting ...
goto :quit
