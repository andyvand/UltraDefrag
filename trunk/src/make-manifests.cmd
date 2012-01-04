@echo off

::
:: This script generates manifests for Vista UAC for UltraDefrag project.
:: Copyright (c) 2009-2012 by Dmitri Arkhangelski (dmitriar@gmail.com).
:: Copyright (c) 2010-2012 by Stefan Pendl (stefanpe@users.sourceforge.net).
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

:: Suggested by Kerem Gumrukcu (http://entwicklung.junetz.de/).

if "%1" equ "" (
    echo Processor Architecture must be specified!
    exit /B 1
)

call :make_manifest %1 1.0.0.0          hibernate    "Hibernate for Windows"         >.\obj\hibernate\hibernate.manifest
call :make_manifest %1 %ULTRADFGVER%.0  udefrag      "UltraDefrag console interface" >.\obj\console\defrag.manifest
call :make_manifest %1 %ULTRADFGVER%.0  ultradefrag  "UltraDefrag GUI"               >.\obj\gui\res\ultradefrag.manifest
call :make_manifest %1 %ULTRADFGVER%.0  bootexctrl   "BootExecute Control Program"   >.\obj\bootexctrl\bootexctrl.manifest
call :make_manifest %1 5.1.2.0          Lua          "Lua Console"                   >.\obj\lua5.1\lua.manifest

rem update manifests in working copy of sources
if /i "%1" equ "X86" (
    copy /Y .\obj\hibernate\hibernate.manifest .\hibernate\hibernate.manifest
    copy /Y .\obj\console\defrag.manifest .\console\defrag.manifest
    copy /Y .\obj\gui\res\ultradefrag.manifest .\gui\res\ultradefrag.manifest
    copy /Y .\obj\bootexctrl\bootexctrl.manifest .\bootexctrl\bootexctrl.manifest
    copy /Y .\obj\lua5.1\lua.manifest .\lua5.1\lua.manifest
    rem copy /Y .\obj\utf8-16\utf8-16.manifest .\utf8-16\utf8-16.manifest
)
exit /B 0

rem Synopsis: call :make_manifest {arch} {version} {app_name} {full_app_name}
rem Example:  call :make_manifest ia64 1.0.0.0 app "Application Name"
:make_manifest
    echo ^<?xml version="1.0" encoding="UTF-8" standalone="yes"?^>
    echo ^<assembly xmlns="urn:schemas-microsoft-com:asm.v1" manifestVersion="1.0"^>
    echo  ^<assemblyIdentity version="%2" name="%3" processorArchitecture="%1" type="win32"/^>
    echo   ^<description^>%~4^</description^>
    echo   ^<dependency^>
    echo    ^<dependentAssembly^>
    echo     ^<assemblyIdentity type="win32" name="Microsoft.Windows.Common-Controls"
    echo       version="6.0.0.0" processorArchitecture="%1"
    echo       publicKeyToken="6595b64144ccf1df" language="*" /^>
    echo    ^</dependentAssembly^>
    echo   ^</dependency^>
    echo   ^<trustInfo xmlns="urn:schemas-microsoft-com:asm.v2"^>
    echo    ^<security^>
    echo     ^<requestedPrivileges xmlns="urn:schemas-microsoft-com:asm.v3"^>
    echo      ^<requestedExecutionLevel level="requireAdministrator" uiAccess="false" /^>
    echo     ^</requestedPrivileges^>
    echo    ^</security^>
    echo   ^</trustInfo^>
    echo   ^<asmv3:application xmlns:asmv3="urn:schemas-microsoft-com:asm.v3"^>
    echo    ^<asmv3:windowsSettings xmlns="http://schemas.microsoft.com/SMI/2005/WindowsSettings"^>
    echo     ^<dpiAware^>true^</dpiAware^>
    echo    ^</asmv3:windowsSettings^>
    echo   ^</asmv3:application^>
    echo ^</assembly^>
goto :EOF

rem the following sequence is faster, but misty
::    type manifest.part1 > %1
::    echo version="%3" name="%4" processorArchitecture="%2" >> %1
::    type manifest.part2 >> %1
::    echo %~5 >> %1
::    type manifest.part3 >> %1
::    echo processorArchitecture="%2" >> %1
::    type manifest.part4 >> %1
::goto :EOF
