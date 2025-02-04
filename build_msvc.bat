@echo off
rem launch this from msvc-enabled console

set CXXFLAGS=/std:c++17 /O2 /FC /W4 /WX /nologo
set INCLUDES=/I SDL3\include
set LIBS=SDL3\lib\x64\SDL3.lib Shell32.lib

if "%1"=="PENGER" (
    set CXXFLAGS=%CXXFLAGS% /DPENGER
)
cl.exe %CXXFLAGS% /Fepng2c png2c.c /link Shell32.lib -SUBSYSTEM:console
png2c.exe digits.png digits > digits.h
if "%1"=="PENGER" (
    png2c.exe penger_walk_sheet.png penger > penger_walk_sheet.h
)
cl.exe %CXXFLAGS% %INCLUDES% /Fesowon main.c /link %LIBS% -SUBSYSTEM:windows
