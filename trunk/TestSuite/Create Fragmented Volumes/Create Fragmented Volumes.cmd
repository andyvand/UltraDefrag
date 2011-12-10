@rem 
@echo off

::
:: This script is used to create fragmented test volumes.
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
title UltraDefrag - Test Volume Fragmentation Creator

::
:: it uses the fragmentation utility included in MyDefrag
:: available at http://www.mydefrag.com/
::
for /d %%D in ( "%ProgramFiles%\MyDefrag*" ) do set MyDefragDir=%%~D

if "%MyDefragDir%" == "" (
    echo.
    echo MyDefrag not installed ... aborting!
    echo.
    pause
    goto :EOF
)

::
:: get install language from registry
::
echo.
echo Getting language from registry, please wait ...

set YES=
set InstallLanguage=X
for /f "tokens=3" %%L in ('reg query "HKLM\SYSTEM\CurrentControlSet\Control\Nls\Language" /v InstallLanguage') do set InstallLanguage=%%L

:: German installation
for %%L in (0407 0c07 1407 1007 0807) do if %InstallLanguage% == %%L set YES=J

:: English installation
for %%L in (0409 0809 0c09 2809 1009 2409 3c09 4009 3809 1809 2009 4409 1409 3409 4809 1c09 2c09 3009) do if %InstallLanguage% == %%L set YES=Y

if not "%YES%" == "" goto :SelectVolume
:: the following must be set to the character representing "YES"
:: this is needed to run the format utility without user interaction
echo.
set /p YES="Enter the letter that represents YES in your language [Y]: "
if "%YES%" == "" set YES=Y

:SelectVolume
:: collect volumes that can be used as test volumes
cls
echo.
echo Collecting available volumes ...
echo.
set MenuItem=0
set FoundVolumes=

for /f "tokens=1,6* skip=8" %%D in ('udefrag -l') do call :AddToDriveList %%~D & if not %%~D == %SystemDrive% call :DisplayMenuItem %%~D - "%%~F"

echo.
echo 0 ... EXIT
echo.
set /p SelectedVolume="Select volume to process: "

if "%SelectedVolume%" == "" goto :EOF
if %SelectedVolume% EQU 0 goto :EOF

if %SelectedVolume% LEQ %MenuItem% goto :GetDriveLetter
echo.
echo Selection must be between 0 and %MenuItem% !!!
echo.
pause
goto :SelectVolume

:GetDriveLetter
for /f "tokens=%SelectedVolume%" %%V in ("%FoundVolumes%") do set ProcessVolume=%%~V

:: ask if we like to format the volume
echo.
set /p FormatVolume="Format volume %ProcessVolume%? (Y/[N]) "
if /i not "%FormatVolume%" == "Y" (
    set FormatVolume=N
) else (
    set FormatVolume=Y
)
if "%FormatVolume%" == "N" goto :SelectFragmentationRate

:: collect available volume types
for /f "tokens=2 delims=[" %%V in ('ver') do set test=%%V
for /f "tokens=2" %%V in ('echo %test%') do set test1=%%V
for /f "tokens=1,2 delims=." %%V in ('echo %test1%') do set OSversion=%%V.%%W

:: FAT volumes
set AvailableTypes=FAT FAT32
set SelectionTypes=FAT-FAT32
if %OSversion% GEQ 5.1 (
    set AvailableTypes=%AvailableTypes% exFAT
    set SelectionTypes=%SelectionTypes%-exFAT
)

:: NTFS volumes
set AvailableTypes=%AvailableTypes% NTFS "NTFS compressed" "NTFS mixed"
set SelectionTypes=%SelectionTypes%-NTFS-NTFS compressed-NTFS mixed

:: UDF volumes
if %OSversion% GEQ 6.0 (
    set AvailableTypes=%AvailableTypes% "UDF 1.02" "UDF 1.50" "UDF 2.00" "UDF 2.01" "UDF 2.50" "UDF 2.50 mirror"
    set SelectionTypes=%SelectionTypes%-UDF 1.02-UDF 1.50-UDF 2.00-UDF 2.01-UDF 2.50-UDF 2.50 mirror
)

:SelectVolumeType
cls
echo.
set MenuItem=0
for %%I in (%AvailableTypes%) do call :DisplayMenuItem %%~I

echo.
echo 0 ... EXIT
echo.
set /p SelectedType="Select new volume type: "

if "%SelectedType%" == "" goto :EOF
if %SelectedType% EQU 0 goto :EOF

if %SelectedType% LEQ %MenuItem% goto :GetVolumeType
echo.
echo Selection must be between 0 and %MenuItem% !!!
echo.
pause
goto :SelectVolumeType

:GetVolumeType
for /f "tokens=%SelectedType% delims=-" %%V in ('echo %SelectionTypes%') do set SelectedVolumeType=%%~V

:SelectFreeSpace
cls
echo.
echo Values 1 to 9 will be multiplied by 10
echo any other value will be used as is

echo.
echo 0 ... EXIT
echo.
set /p value="Enter percentage of free Space: "

if "%value%" == "" goto :EOF
if %value% EQU 0 goto :EOF

set PercentageFree=0

if %value% LEQ 100 set PercentageFree=%value%
if %value% LSS 10 set /a PercentageFree="value * 10"

if %PercentageFree% GTR 0 goto :SelectSmallFileRate

echo.
echo Selection must be between 0 and 100 !!!
echo.
pause
goto :SelectFreeSpace

:SelectSmallFileRate
echo.
echo --------------------------------------
echo.
echo Enter the small files rate,
echo x out of 10 files should be below 512kB.
echo.
set /p SmallFileRate="Enter a value between 1 and 10 for x: "

if "%SmallFileRate%" == "" set SmallFileRate=1
if %SmallFileRate% LEQ 0 set SmallFileRate=1
if %SmallFileRate% GTR 10 set SmallFileRate=10

:SelectFragmentationRate
echo.
echo --------------------------------------
echo.
echo Enter the fragmentation rate,
echo every x-th file should be fragmented.
echo.
set /p FragmentationRate="Enter a value between 1 and 100 for x: "

if "%FragmentationRate%" == "" set FragmentationRate=1
if %FragmentationRate% LEQ 0 set FragmentationRate=1
if %FragmentationRate% GTR 100 set FragmentationRate=100

call :delay 0

if "%FormatVolume%" == "N" goto :ParseVolumeLabel

for /f "tokens=1,2,3" %%R in ('echo %SelectedVolumeType%') do (
    set ex_type=%%R
    set option1=%%S
    set option2=%%T
)

set VolumeName=%ex_type%
if not "%option1%" == "" set VolumeName=%VolumeName%_%option1:.=%
if not "%option2%" == "" set VolumeName=%VolumeName%_%option2%
set VolumeName=%VolumeName%_sr%SmallFileRate%_fr%FragmentationRate%

call :answers >"%TMP%\answers.txt"

title Setting Volume Label of "%ProcessVolume%" ...
echo.
set CommandLine=label %ProcessVolume% TEST
echo %CommandLine%
echo.
%CommandLine%

call :delay 2

title Formatting Drive "%ProcessVolume%" ...

set Switches=

if not "%ex_type%" == "FAT" if not "%ex_type%" == "FAT32" if not "%ex_type%" == "exFAT" goto :NTFS
goto :DoFormat

:NTFS
if not "%ex_type%" == "NTFS" goto :UDF
if "%option1%" == "compressed" set Switches=/C
goto :DoFormat

:UDF
set Switches=/R:%option1%
if "%option2%" == "mirror" set Switches=%Switches% /D

:DoFormat
echo.
set CommandLine=format %ProcessVolume% /FS:%ex_type% /V:%VolumeName% %Switches% /X
echo %CommandLine%
echo.
%CommandLine% <"%TMP%\answers.txt"

call :delay 5

goto :StartProcess

:ParseVolumeLabel
for /f "tokens=1,5,6* skip=8" %%D in ('udefrag -l') do if %%~D == %ProcessVolume% set PercentageFree=%%E & set VolumeName="%%~G"

for /f "tokens=1,2,3,4 delims=_" %%R in ('echo %VolumeName:"=%') do (
    set ex_type=%%R
    set option1=%%S
    set option2=%%T
    set option3=%%U
)

set ApplyLabel=0

if not "%ex_type%" == "FAT" if not "%ex_type%" == "FAT32" if not "%ex_type%" == "exFAT" goto :makeNTFS
set ApplyLabel=1
set VolumeName=%ex_type%

:makeNTFS
if not "%ex_type%" == "NTFS" goto :makeUDF
set ApplyLabel=1
set VolumeName=%ex_type%
if not "%option1%" == "compressed" if not "%option1%" == "mixed" goto :makeUDF
set VolumeName=%VolumeName%_%option1%

:makeUDF
if not "%ex_type%" == "UDF" goto :ApplyVolumeLabel
set ApplyLabel=1
set VolumeName=%ex_type%
set VolumeName=%VolumeName%_%option1:.=%
if "%option2%" == "mirror" set VolumeName=%VolumeName%_%option2%

:ApplyVolumeLabel
if %ApplyLabel% EQU 0 goto :StartProcess
set temp_var=%option1:~0,2%
if "%temp_var%" == "sr" set SmallFileRate=%option1:~2%
set temp_var=%option2:~0,2%
if "%temp_var%" == "sr" set SmallFileRate=%option2:~2%
set temp_var=%option3:~0,2%
if "%temp_var%" == "sr" set SmallFileRate=%option3:~2%
set VolumeName=%VolumeName%_sr%SmallFileRate%_fr%FragmentationRate%

title Setting Volume Label of "%ProcessVolume%" ...
echo.
set CommandLine=label %ProcessVolume% %VolumeName%
echo %CommandLine%
echo.
%CommandLine%

call :delay 2

:StartProcess
rem process the volume
call :FragmentDrive "%ProcessVolume%"

title Operation Completed ...

:quit
echo.
pause

goto :EOF

:DisplayMenuItem
    set /a MenuItem+=1
    echo %MenuItem% ... %*
goto :EOF

:AddToDriveList
    if %~1 == %SystemDrive% goto :EOF
    
    if "%FoundVolumes%" == "" (
        set FoundVolumes=%~1
    ) else (
        set FoundVolumes=%FoundVolumes% %~1
    )
goto :EOF

:FragmentDrive
    title Checking Drive "%~1" ...
    echo Executing ... chkdsk %~1 /r /f
    echo. & echo %YES% | chkdsk %~1 /r /f
    
    call :delay 2
    
    set /a size="24 + %RANDOM% / 3"
    set /a fragments="%RANDOM% / 1365"
    set count=0
    set NoCompr=0
    set dest=%~1
    set ExitCode=0
    
    if "%FormatVolume%" == "Y" goto :create
    
    title Changing Fragmented Files on Drive "%~1 (%VolumeName%)" ...
    echo Changing Fragmented Files on Drive "%~1 (%VolumeName%)" ...
    echo.

    for /r "%~1" %%X in ( *.* ) do (
        call :doFragment "%%~X" "%%~zX" || goto :finished
        call :increment
        ping -n 3 localhost >NUL
    )
    
    goto :finished

    :create
    title Creating Fragmented Files on Drive "%~1 (%VolumeName%)" until %PercentageFree%%% free space left ...
    echo Creating Fragmented Files on Drive "%~1 (%VolumeName%)" until %PercentageFree%%% free space left ...
    echo.

    :loop
        call :doit "%~1" || goto :finished
        call :increment
        ping -n 3 localhost >NUL
    for /f "tokens=1,5 skip=8" %%X in ( 'udefrag -l' ) do if "%%~X" == "%~1" if %PercentageFree% LEQ %%Y goto :loop

    :finished
    if %ExitCode% GTR 0 (
        echo.
        echo Operation failed ...
    ) else (
        echo.
        echo Operation succeeded ...
    )
    
    call :delay 5
    
    title Checking Drive "%~1" ...
    echo Executing ... chkdsk %~1 /r /f
    echo. & echo %YES% | chkdsk %~1 /r /f
    
    call :delay 2
goto :EOF

:answers
    echo TEST
    echo %YES%
goto :EOF

:doFragment
    set /a size="%~2 / 1024"
    
    set count_fmt=   %count%
    set size_fmt=      %size%
    set frag_fmt=   %fragments%

    echo File %count_fmt:~-3% ... %size_fmt:~-6% kB ... %frag_fmt:~-3% Fragments ... "%~1"

    "%MyDefragDir%\MyFragmenter.exe" -p %fragments% "%~1" >NUL
    set ExitCode=%ERRORLEVEL%
    exit /B %ExitCode%
goto :EOF

:doit
    if %count% EQU 0 goto :skip
        if %NoFolder% EQU 0 set dest=%~1\folder_%count%
        if %NoFolder% EQU 0 echo Folder                     %dest%
        if %NoFolder% EQU 0 mkdir "%dest%"
    :skip

    set count_fmt=   %count%
    set size_fmt=      %size%
    set frag_fmt=   %fragments%

    echo File %count_fmt:~-3% ... %size_fmt:~-6% kB ... %frag_fmt:~-3% Fragments

    "%MyDefragDir%\MyFragmenter.exe" -p %fragments% -s %size% "%dest%\file_%count%.bin" >NUL
    set ExitCode=%ERRORLEVEL%
    if %ExitCode% GTR 0 exit /B %ExitCode%
    
    if "%option1%" == "mixed" if %NoCompr% EQU 0 compact /c "%dest%\file_%count%.bin" >NUL
    set ExitCode=%ERRORLEVEL%
    exit /B %ExitCode%
goto :EOF

:delay
    set /a seconds="%1 + 1"
    echo.
    if %seconds% == 1 (
        echo ============================================
    ) else (
        echo --------------------------------------------
        ping -n %seconds% localhost >NUL
    )
    echo.
goto :EOF

:increment
    set /a count+=1
    
    set /a NoFolder="count %% 10"
    set /a NoCompr="count %% 5"
    
    if %NoFolder% LSS %SmallFileRate% (
        set divisor=64
    ) else (
        set divisor=3
    )

    set /a size="24 + %RANDOM% / %divisor%"
    set /a fragments="%RANDOM% / 1365"
    
    set /a quotient="count %% %FragmentationRate%"
    
    if %quotient% EQU 0 goto :EOF
    
    set fragments=0
goto :EOF
