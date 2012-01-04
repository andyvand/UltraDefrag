@echo off

::
:: This script upgrades all doxygen configuration files of UltraDefrag project.
:: Copyright (c) 2011-2012 by Stefan Pendl (stefanpe@users.sourceforge.net).
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

echo Upgrading doxygen configuration files...
echo.

call :upgrade_config .                    || goto fail
call :upgrade_config .\dll\udefrag        || goto fail
call :upgrade_config .\dll\wgx            || goto fail
call :upgrade_config .\dll\zenwinx        || goto fail
call :upgrade_config ..\doc\html\handbook || goto fail

echo.
echo Upgrade succeeded!
goto :END

:fail
echo.
echo Upgrade failed!

:END
echo.
pause
goto :EOF

rem Synopsis: call :upgrade_config {path}
rem Example:  call :upgrade_config .\dll\zenwinx
:upgrade_config
    pushd %1
    echo ====
    echo %CD%
    doxygen -u DoxyFile || goto compilation_failed
    
    :compilation_succeeded
    del /f /q DoxyFile.bak
    popd
    exit /B 0
    :compilation_failed
    popd
    exit /B 1
goto :EOF
