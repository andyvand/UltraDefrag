@echo off

echo This script will replace the libntdll.a file
echo in MinGW installation with a more adequate version.
echo Usage: mingw_patch.cmd {path to mingw installation}
echo.

if "%1" equ "" (
    exit /B 0
)

echo Preparing to patch applying...
pushd %~dp0

:: save original version
if not exist %1\lib\libntdll.a.orig (
    move %1\lib\libntdll.a %1\lib\libntdll.a.orig
)

:: generate more adequate version
%1\bin\dlltool -k --output-lib libntdll.a --def ntdll.def
if %errorlevel% neq 0 goto fail

:: replace file
move /Y libntdll.a %1\lib\libntdll.a

echo MinGW patched successfully!
popd
exit /B 0

:fail
echo Build error (code %ERRORLEVEL%)!
popd
exit /B 1
