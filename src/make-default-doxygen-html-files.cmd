@echo off

::
:: This script creates default set of auxiliary files for doxygen docs.
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

echo Creating default doxygen HTML header, footer and CSS script...
echo.

call :make_default .                    || goto fail
call :make_default .\dll\udefrag        || goto fail
call :make_default .\dll\wgx            || goto fail
call :make_default .\dll\zenwinx        || goto fail
call :make_default ..\doc\html\handbook || goto fail

call :make_pdf_default ..\doc\html\handbook a4     || goto fail
call :make_pdf_default ..\doc\html\handbook letter || goto fail

echo.
echo Created default files successfully!
goto :END

:fail
echo.
echo Creating default files failed!

:END
echo.
pause
goto :EOF

rem Synopsis: call :make_default {path}
rem Example:  call :make_default .\dll\zenwinx
:make_default
    pushd %1
    echo ====
    echo %CD%
    echo.
    if exist doxy-defaults rd /s /q doxy-defaults
    doxygen -w html default_header.html default_footer.html default_styles.css DoxyFile || goto compilation_failed
    md doxy-defaults
    move /Y default_*.* doxy-defaults
    
    :compilation_succeeded
    popd
    exit /B 0
    :compilation_failed
    popd
    exit /B 1
goto :EOF

rem Synopsis: call :make_pdf_default {path} {paper size}
rem Example:  call :make_pdf_default ..\doc\html\handbook letter
:make_pdf_default
    pushd %1
    echo ====
    echo %CD%
    echo.
    if exist doxy-defaults_%2 rd /s /q doxy-defaults_%2

    copy /v /y Doxyfile Doxyfile_%2
    (
        echo GENERATE_HTML          = NO
        echo GENERATE_LATEX         = YES
        echo LATEX_OUTPUT           = latex_%2
        echo PAPER_TYPE             = %2
    ) >>Doxyfile_%2

    doxygen -w latex default_header.tex default_footer.tex default_styles.sty DoxyFile_%2 || goto compilation_failed
    md doxy-defaults_%2
    move /Y default_*.* doxy-defaults_%2
    
    :compilation_succeeded
    del /f /q Doxyfile_%2
    popd
    exit /B 0
    :compilation_failed
    del /f /q Doxyfile_%2
    popd
    exit /B 1
goto :EOF
