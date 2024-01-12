@echo off

rem SPDX-License-Identifier: GPL-3.0-or-later
rem Copyright (C) 2023 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>

set CFLAGS=/nologo /TC /Zi /Od

set CFLAGS=%CFLAGS% /I include /I vendor\afeirsa\include
set CFLAGS=%CFLAGS% /I vendor\www\Library\Implementation /I vendor\support
set CFLAGS=%CFLAGS% /I vendor\python\src

set CFLAGS=%CFLAGS% /D_DEBUG /DAF_WGL /DGL10_COMPAT /DNO_EXT /D_WINDOWS
set CFLAGS=%CFLAGS% /DNO_UNIX_IO /DNO_GROUPS /DUSE_STDLIB

set LDFLAGS=/nologo /link /nologo /subsystem:windows /FORCE:UNRESOLVED
set LDFLAGS=%LDFLAGS% /DEBUG:FULL
set LDFLAGS=%LDFLAGS% opengl32.lib glu32.lib shell32.lib gdi32.lib user32.lib
set LDFLAGS=%LDFLAGS% Ws2_32.lib
rem windres -i res\aga.rc -o res\aga.res
rem windres -i res\aga.rc -o res\aga.o

set PGENDIR=vendor\support
cl.exe %CFLAGS% %PGENDIR%\pgen_unity.c /Fe:%PGENDIR%\pgenmain.exe

set OBJTMP="%TMP%\bat~%RANDOM%.tmp"

for /f "delims=" %%a in ('dir /s/b *.c') do call :cc %%a %%~pa%%~na.obj
goto :done

:cc
    set CALL="cl.exe /c %CFLAGS% /DAF_BUILD %1 /Fo:%2"
    echo %CALL%
    cmd.exe /c %CALL%
    echo %2 >> %OBJTMP%
goto :EOF

:done

set CALL="cl.exe /Fe:%cd%\src\main.exe @%OBJTMP% %LDFLAGS%"
echo %CALL%
cmd.exe /c %CALL%
