@echo off

::
:: Build script for the UltraDefrag project documentation.
:: Copyright (c) 2007-2012 Dmitri Arkhangelski (dmitriar@gmail.com).
:: Copyright (c) 2010-2012 Stefan Pendl (stefanpe@users.sourceforge.net).
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

doxygen --version >nul 2>&1 || goto fail

call :compile_docs .                                 || goto fail
call :compile_docs .\dll\udefrag        udefrag.dll  || goto fail
call :compile_docs .\dll\wgx            wgx          || goto fail
call :compile_docs .\dll\zenwinx                     || goto fail
call :compile_docs ..\doc\html\handbook              || goto fail

:: compile PDF documentation if MiKTeX is installed
pdflatex --version >nul 2>&1
if not errorlevel 1 (
    mkdir .\release
    call :compile_pdf ..\doc\html\handbook a4     UltraDefrag_Handbook || goto fail
    call :compile_pdf ..\doc\html\handbook letter UltraDefrag_Handbook || goto fail
)

:: move .htaccess file to the root of dev docs
move /Y doxy-doc\html\.htaccess doxy-doc\.htaccess

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

    if exist .\rsc\sflogo.gif copy /Y .\rsc\sflogo.gif .\doxy-doc\html\
    if exist .\rsc\.htaccess  copy /Y .\rsc\.htaccess  .\doxy-doc\html\
    if exist .\rsc\custom-doxygen.css  copy /Y .\rsc\custom-doxygen.css  .\doxy-doc\html\

    del /Q .\doxy-doc\html\doxygen.png

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

rem Synopsis: call :compile_pdf {path} {paper size} {PDF name}
rem Example:  call :compile_pdf ..\doc\html\handbook letter UltraDefrag_Handbook
:compile_pdf
    pushd %1
    rd /s /q doxy-doc\latex_%2
    lua "%~dp0\tools\set-doxyfile-project-number.lua" Doxyfile %ULTRADFGVER% || goto compilation_failed

    copy /v /y Doxyfile Doxyfile_%2
    (
        echo GENERATE_HTML          = NO
        echo GENERATE_LATEX         = YES
        echo LATEX_OUTPUT           = latex_%2
        echo PAPER_TYPE             = %2
    ) >>Doxyfile_%2
    
    doxygen Doxyfile_%2 || goto compilation_failed

    copy /Y .\rsc\*.png .\doxy-doc\latex_%2

    :compilation_succeeded
    pushd doxy-doc\latex_%2

    echo.
    pdflatex  refman
    echo ----
    echo.
    makeindex refman.tex
    echo ----
    echo.
    pdflatex  refman

    setlocal enabledelayedexpansion
    set count=5
    :repeat
        set content=X
        for /F "tokens=*" %%T in ( 'findstr /C:"Rerun LaTeX" refman.log' ) do set content="%%~T"
        if !content! == X for /F "tokens=*" %%T in ( 'findstr /C:"Rerun to get cross-references right" refman.log' ) do set content="%%~T"
        if !content! == X goto :skip
        set /a count-=1
        if !count! EQU 0 goto :skip

        echo ----
        echo.
        pdflatex  refman
    goto :repeat
    :skip
    endlocal

    if "%3" neq "" copy /Y refman.pdf "%~dp0\release\%3_%ULTRADFGVER%_%2.pdf" || goto compilation_failed

    popd
    del /f /q Doxyfile_%2
    popd
    exit /B 0
    :compilation_failed
    popd
    del /f /q Doxyfile_%2
    popd
    exit /B 1
goto :EOF
