@echo off

::
:: Build script for the UltraDefrag project documentation.
:: Copyright (c) 2007-2013 Dmitri Arkhangelski (dmitriar@gmail.com).
:: Copyright (c) 2010-2013 Stefan Pendl (stefanpe@users.sourceforge.net).
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

call ParseCommandLine.cmd %*

:: set environment variables if they aren't already set
if "%ULTRADFGVER%" equ "" (
    call setvars.cmd
    if exist "setvars_%COMPUTERNAME%_%ORIG_USERNAME%.cmd" call "setvars_%COMPUTERNAME%_%ORIG_USERNAME%.cmd"
    if exist "setvars_%COMPUTERNAME%_%USERNAME%.cmd" call "setvars_%COMPUTERNAME%_%USERNAME%.cmd"
)

doxygen --version >nul 2>&1 || goto fail

if "%UD_BLD_FLG_BUILD_DEV%" == "1" (
    call :compile_docs .                    .            || goto fail
    call :compile_docs .\dll\udefrag        udefrag.dll  || goto fail
    call :compile_docs .\dll\wgx            wgx          || goto fail
    call :compile_docs .\dll\zenwinx                     || goto fail
)

if "%UD_BLD_FLG_BUILD_INT%" == "1" (
    rd /s /q "%~dp0\internal-doc\%ULTRADFGVER%"
    md "%~dp0\internal-doc\%ULTRADFGVER%"

    call :compile_int .\dll\udefrag        udefrag.dll  || goto fail
    call :compile_int .\dll\wgx            wgx          || goto fail
    call :compile_int .\dll\zenwinx        zenwinx      || goto fail
    call :compile_int .\gui                gui          || goto fail
) else (
    call :compile_docs ..\doc\handbook || goto fail
    copy /Y ..\doc\handbook\doxy-doc\html\*.* "%~dp0\..\..\web\handbook" || goto fail
)

:: compile PDF documentation if MiKTeX is installed
if "%UD_BLD_FLG_BUILD_PDF%" == "1" (
    pdflatex --version >nul 2>&1
    if not errorlevel 1 (
        mkdir .\release
        call :compile_pdf ..\doc\handbook a4     UltraDefrag_Handbook || goto fail
        call :compile_pdf ..\doc\handbook letter UltraDefrag_Handbook || goto fail
    )
)

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

    del /Q .\doxy-doc\html\doxygen.png

    if "%2" neq "" (
        copy /Y .\doxy-doc\html\*.* "%~dp0\..\..\web\doxy-doc\%2\html" || goto compilation_failed
    )
    :compilation_succeeded
    popd
    exit /B 0
    :compilation_failed
    popd
    exit /B 1
goto :EOF

rem Synopsis: call :compile_pdf {path} {paper size} {PDF name}
rem Example:  call :compile_pdf ..\doc\handbook letter UltraDefrag_Handbook
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
        echo LATEX_HEADER           = ./rsc/custom_header_%2.tex
    ) >>Doxyfile_%2
    
    doxygen Doxyfile_%2 || goto compilation_failed

    copy /Y .\rsc\*.png .\doxy-doc\latex_%2
    copy /Y .\rsc\*.sty .\doxy-doc\latex_%2

    :compilation_succeeded
    pushd doxy-doc\latex_%2

    call make.bat

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

rem Synopsis: call :compile_int {path} {name}
rem Example:  call :compile_int .\dll\zenwinx zenwinx
:compile_int
    pushd %1

    (
        echo PROJECT_NAME           = %2
        echo PROJECT_NUMBER         = %ULTRADFGVER%
        echo OUTPUT_DIRECTORY       = "%~dp0\internal-doc\%ULTRADFGVER%"
        echo FULL_PATH_NAMES        = NO
        echo INHERIT_DOCS           = NO
        echo OPTIMIZE_OUTPUT_FOR_C  = YES
        echo EXTRACT_LOCAL_CLASSES  = NO
        echo HIDE_UNDOC_MEMBERS     = YES
        echo HIDE_UNDOC_CLASSES     = YES
        echo INTERNAL_DOCS          = YES
        echo CASE_SENSE_NAMES       = YES
        echo SORT_MEMBER_DOCS       = NO
        echo INPUT                  = .
        echo FILE_PATTERNS          = *.c *.dox
        echo SOURCE_BROWSER         = YES
        echo REFERENCED_BY_RELATION = YES
        echo REFERENCES_RELATION    = YES
        echo VERBATIM_HEADERS       = NO
        echo HTML_OUTPUT            = %2
        echo HTML_TIMESTAMP         = NO
        echo GENERATE_TREEVIEW      = YES
        echo GENERATE_LATEX         = NO
        echo ENABLE_PREPROCESSING   = NO
        echo INCLUDE_FILE_PATTERNS  = *.h
    ) >Doxyfile_internal
    
    doxygen Doxyfile_internal || goto compilation_failed

    :compilation_succeeded
    del /f /q Doxyfile_internal
    popd
    exit /B 0
    :compilation_failed
    del /f /q Doxyfile_internal
    popd
    exit /B 1
goto :EOF
