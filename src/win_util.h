// win_util.h - the only place allowed to know about windows.h.
// Keeping it out of main.cpp matters: <windows.h> defines Rectangle, CloseWindow,
// DrawText, LoadImage and ShowCursor, which collide with raylib's names.
#pragma once

void show_error_box(const char* title, const char* text);
