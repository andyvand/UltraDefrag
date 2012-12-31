@echo off

:: This script was made for myself (Dmitri Arkhangelski)
:: to simplify binary packages uploading.
::
:: Copyright (c) 2007-2012 Dmitri Arkhangelski (dmitriar@gmail.com).
:: Copyright (c) 2011-2012 Stefan Pendl (stefanpe@users.sourceforge.net).
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

call build.cmd --clean

rd /s /q release
mkdir release

:: build source code package
call build-src-package.cmd || goto build_failed
copy .\src_package\ultradefrag-%UDVERSION_SUFFIX%.src.7z .\release\

:: build all 32-bit packages
call build.cmd --all --use-mingw || goto build_failed
copy .\bin\ultradefrag-%UDVERSION_SUFFIX%.bin.i386.exe .\release\
copy .\bin\ultradefrag-portable-%UDVERSION_SUFFIX%.bin.i386.zip .\release\

:: build all 64-bit packages
call build.cmd --all --use-winsdk --no-x86 || goto build_failed
copy .\bin\amd64\ultradefrag-%UDVERSION_SUFFIX%.bin.amd64.exe .\release\
copy .\bin\ia64\ultradefrag-%UDVERSION_SUFFIX%.bin.ia64.exe .\release\
copy .\bin\amd64\ultradefrag-portable-%UDVERSION_SUFFIX%.bin.amd64.zip .\release\
copy .\bin\ia64\ultradefrag-portable-%UDVERSION_SUFFIX%.bin.ia64.zip .\release\

cd release
..\tools\md5sum ultradefrag-%UDVERSION_SUFFIX%.bin.*           > ultradefrag-%UDVERSION_SUFFIX%.MD5SUMS
..\tools\md5sum ultradefrag-portable-%UDVERSION_SUFFIX%.bin.* >> ultradefrag-%UDVERSION_SUFFIX%.MD5SUMS
..\tools\md5sum ultradefrag-%UDVERSION_SUFFIX%.src.*          >> ultradefrag-%UDVERSION_SUFFIX%.MD5SUMS
..\tools\md5sum UltraDefrag_Handbook_%ULTRADFGVER%_*.pdf      >> ultradefrag-%UDVERSION_SUFFIX%.MD5SUMS
cd ..

:: update history file
copy .\HISTORY.TXT .\release\
copy /Y .\HISTORY.TXT ..\..\web\

:: update version notification
echo %ULTRADFGVER% > ..\..\web\version.ini
echo %ULTRADFGVER% > ..\..\web\version_xp.ini

echo.
echo Release made successfully!
title Release made successfully!
exit /B 0

:build_failed
echo.
echo Release building error!
title Release building error!
exit /B 1
