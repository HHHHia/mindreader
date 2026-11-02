@echo off
setlocal
set ROOT=%~dp0
set GXX=g++
if exist "D:\64devkit\w64devkit\bin\g++.exe" set GXX=D:\64devkit\w64devkit\bin\g++.exe

"%GXX%" -std=c++20 -O2 -Wall -Wextra "%ROOT%src\core.cpp" "%ROOT%tests\test_core.cpp" -o "%ROOT%tests\test_core.exe"

if errorlevel 1 (
  echo BUILD FAILED
  pause
  exit /b 1
)

"%ROOT%tests\test_core.exe"
pause
