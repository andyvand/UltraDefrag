@echo off

::
:: Build script for the UltraDefrag project documentation.
:: Copyright (c) 2007-2012 by Dmitri Arkhangelski (dmitriar@gmail.com).
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

echo Build UltraDefrag development docs...
echo.

:: set environment variables if they aren't already set
if "%ULTRADFGVER%" equ "" (
    call setvars.cmd
    if exist "setvars_%COMPUTERNAME%_%ORIG_USERNAME%.cmd" call "setvars_%COMPUTERNAME%_%ORIG_USERNAME%.cmd"
    if exist "setvars_%COMPUTERNAME%_%USERNAME%.cmd" call "setvars_%COMPUTERNAME%_%USERNAME%.cmd"
)

call :compile_docs .                                 || goto fail
call :compile_docs .\dll\udefrag        udefrag.dll  || goto fail
call :compile_docs .\dll\wgx            wgx          || goto fail
call :compile_docs .\dll\zenwinx                     || goto fail
call :compile_docs ..\doc\html\handbook              || goto fail

:: move .htaccess file to the root of dev docs
move /Y doxy-doc\html\.htaccess doxy-doc\.htaccess

:: clean up the handbook
pushd ..\doc\html\handbook\doxy-doc\html
del /Q  header.html, footer.html
del /Q  doxygen.png
del /Q  tabs.css
del /Q  bc_*.png
del /Q  nav_*.png
del /Q  open.png
del /Q  closed.png
popd

echo.
echo Docs compiled successfully!
exit /B 0

:fail
echo.
echo Docs compilation failed!
exit /B 1

rem Synopsis: call :compile_docs {path} {name}
rem Note:     omit the second parameter to prevent copying docs to /src/doxy-doc directory
rem Example:  call :compile_docs .\dll\zenwinx zenwinx
:compile_docs
    pushd %1
    rd /s /q doxy-doc
    lua "%~dp0\tools\set-doxyfile-project-number.lua" Doxyfile %ULTRADFGVER% || goto compilation_failed
    doxygen || goto compilation_failed
    copy /Y .\rsc\*.* .\doxy-doc\html\
    if "%2" neq "" (
        xcopy /I /Y /Q /S .\doxy-doc\html "%~dp0\doxy-doc\%2\html" || goto compilation_failed
    )
    :compilation_succeeded
    popd
    exit /B 0
    :compilation_failed
    popd
    exit /B 1
goto :EOF
