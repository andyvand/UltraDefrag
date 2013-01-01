@echo off

echo This script puts ntdll.lib files to appropriate 
echo directories of Windows SDK which misses them.
echo Usage: winsdk_patch.cmd {path to sdk installation}
echo.

if "%~1" equ "" (
    exit /B 1
)

echo Preparing to patch applying...

pushd "%~dp0"
copy /Y ntdll\ntdll.lib "%~1\Lib\"
copy /Y ntdll\amd64\ntdll.lib "%~1\Lib\x64"
copy /Y ntdll\ia64\ntdll.lib "%~1\Lib\IA64"
popd

echo Windows SDK patched successfully!
exit /B 0
