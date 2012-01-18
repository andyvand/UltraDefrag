@echo off

::
:: This script is used to set the initial build defaults,
:: which are changed by the command line switches described
:: in "build-help.cmd"
::
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


echo.
echo %~n0 ... %*
echo.

:: set to 1 to debug parameter checking
set UD_BLD_FLG_PARSER_DEBUG=0

:: set compiler constants
set UD_BLD_FLG_USE_MINGW=1
set UD_BLD_FLG_USE_MINGW64=3
set UD_BLD_FLG_USE_WINDDK=4
set UD_BLD_FLG_USE_WINSDK=5

:: set defaults
set UD_BLD_FLG_USE_COMPILER=0
set UD_BLD_FLG_DO_INSTALL=0
set UD_BLD_FLG_ONLY_CLEANUP=0
set UD_BLD_FLG_DIPLAY_HELP=0
set UD_BLD_FLG_IS_PORTABLE=0
set UD_BLD_FLG_BUILD_X86=1
set UD_BLD_FLG_BUILD_AMD64=0
set UD_BLD_FLG_BUILD_IA64=0
set UD_BLD_FLG_BUILD_ALL=0

:ParseArgs
if "%1" == "--use-mingw" (
    set UD_BLD_FLG_USE_COMPILER=%UD_BLD_FLG_USE_MINGW%
    set UD_BLD_FLG_BUILD_X86=1
    set UD_BLD_FLG_BUILD_AMD64=0
    set UD_BLD_FLG_BUILD_IA64=0
)
if "%1" == "--use-winddk" (
    set UD_BLD_FLG_USE_COMPILER=%UD_BLD_FLG_USE_WINDDK%
    set UD_BLD_FLG_BUILD_X86=1
    set UD_BLD_FLG_BUILD_AMD64=1
    set UD_BLD_FLG_BUILD_IA64=1
)
if "%1" == "--use-winsdk" (
    set UD_BLD_FLG_USE_COMPILER=%UD_BLD_FLG_USE_WINSDK%
    set UD_BLD_FLG_BUILD_X86=1
    set UD_BLD_FLG_BUILD_AMD64=1
    set UD_BLD_FLG_BUILD_IA64=1
)
if "%1" == "--use-mingw-x64" (
    set UD_BLD_FLG_USE_COMPILER=%UD_BLD_FLG_USE_MINGW64%
    set UD_BLD_FLG_BUILD_X86=0
    set UD_BLD_FLG_BUILD_AMD64=1
    set UD_BLD_FLG_BUILD_IA64=0
)
if "%1" == "--install"      set UD_BLD_FLG_DO_INSTALL=1
if "%1" == "--clean"        set UD_BLD_FLG_ONLY_CLEANUP=1
if "%1" == "--help"         set UD_BLD_FLG_DIPLAY_HELP=1
if "%1" == "--portable"     set UD_BLD_FLG_IS_PORTABLE=1
if "%1" == "--all"          set UD_BLD_FLG_BUILD_ALL=1
if "%1" == "--no-x86"       set UD_BLD_FLG_BUILD_X86=0
if "%1" == "--no-amd64"     set UD_BLD_FLG_BUILD_AMD64=0
if "%1" == "--no-ia64"      set UD_BLD_FLG_BUILD_IA64=0

shift
if not "%1" == "" goto :ParseArgs

:: skip next section if not debugging command line parser
if %UD_BLD_FLG_PARSER_DEBUG% == 0 goto :EOF

:: display variables
set ud

:: clear variables
for %%V in ( UD_BLD_FLG_USE_COMPILER UD_BLD_FLG_DO_INSTALL UD_BLD_FLG_ONLY_CLEANUP UD_BLD_FLG_DIPLAY_HELP ) do set %%V=
for %%V in ( UD_BLD_FLG_IS_PORTABLE UD_BLD_FLG_BUILD_ALL ) do set %%V=
for %%V in ( UD_BLD_FLG_BUILD_X86 UD_BLD_FLG_BUILD_AMD64 UD_BLD_FLG_BUILD_IA64 ) do set %%V=
