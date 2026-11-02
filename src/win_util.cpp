// win_util.cpp - MessageBox behind a wall, so raylib never sees <windows.h>.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include "win_util.h"

void show_error_box(const char* title, const char* text)
{
    MessageBoxA(nullptr, text, title, MB_OK | MB_ICONERROR);
}
