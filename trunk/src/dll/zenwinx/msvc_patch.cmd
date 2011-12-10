@echo off

echo.
echo This script contains information how to patch
echo MS Visual Studio 6.0 manually to make it possible
echo to compile UltraDefrag binaries by it.
echo.
echo * Download ntdll.lib file and put it to MSVS Lib subdirectory:
echo.
echo     http://www.masm32.com/board/index.php?topic=2124.new
echo.
echo * Replace the following lines in winnt.h file:
echo.
echo     #if defined(_M_ALPHA)
echo     #define NtCurrentTeb() ((struct _TEB *)_rdteb())
echo     #else
echo     struct _TEB *
echo     NtCurrentTeb(void);
echo     #endif
echo.
echo by the following correct definition:
echo.
echo     #if defined(_M_ALPHA)
echo     #define NtCurrentTeb() ((struct _TEB *)_rdteb())
echo     #else
echo     struct _TEB *
echo     __stdcall NtCurrentTeb(void);
echo     #endif
echo.
echo That's all ;)

exit /B 0
