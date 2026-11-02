@echo off
setlocal
set ROOT=%~dp0
set RAYLIB=%ROOT%third_party\raylib-6.2-patched

rem put the compiler you actually have here
set GXX=g++
if exist "D:\64devkit\w64devkit\bin\g++.exe" set GXX=D:\64devkit\w64devkit\bin\g++.exe

if not exist "%ROOT%data\players" mkdir "%ROOT%data\players"

rem -mwindows: no console window, this is a GUI program
"%GXX%" -std=c++20 -O2 -Wall -Wextra "%ROOT%src\core.cpp" "%ROOT%src\win_util.cpp" "%ROOT%src\main.cpp" -o "%ROOT%mindreader.exe" -I"%RAYLIB%\include" -L"%RAYLIB%\lib" -static -lraylib -lopengl32 -lgdi32 -lwinmm -mwindows

if errorlevel 1 (
  echo.
  echo BUILD FAILED
  pause
  exit /b 1
)

echo.
echo BUILD OK -^> mindreader.exe
pause
