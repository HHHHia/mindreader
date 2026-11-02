# raylib 6.2, patched

Everything here is raylib 6.2 built from source with **one** change, because the
stock build refuses to start on this machine.

## The problem

`InitWindow()` died with `PLATFORM: Failed to initialize graphics device`.
Two separate bugs stacked on top of each other:

1. `DirectInput8Create()` returns `E_FAIL` on this machine. GLFW uses it for
   joystick support during `glfwInit()`, and treats the failure as fatal, so the
   whole platform init failed before any window was created.
2. Even after making (1) non-fatal, raylib still refused to start: right after
   creating the window it calls `glfwGetError(NULL)` and reads *any* pending
   error as "the OpenGL context never became current". The leftover DirectInput
   error was still queued, so the check failed.

## The patch

`src/external/glfw/src/win32_joystick.c`, inside `_glfwInitJoysticksWin32()`:

```c
if (FAILED(DirectInput8Create(...)))
{
    _glfw.win32.dinput8.api = NULL;                     // no joysticks
    _glfwInputError(GLFW_PLATFORM_ERROR,
                    "Win32: Failed to create interface (joysticks disabled)");
    glfwGetError(NULL);                                 // clear the pending error
    return GLFW_TRUE;                                   // keep going
}
```

The stock code logged the error and returned `GLFW_FALSE`.

## Rebuilding it

1. Download <https://github.com/raysan5/raylib/archive/refs/tags/6.2.zip>
2. Apply the patch above to `src/external/glfw/src/win32_joystick.c`
3. Build with MinGW:

```
cmake -S raylib-6.2 -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DBUILD_EXAMPLES=OFF -DBUILD_SHARED_LIBS=OFF
cmake --build build -j
```

4. Copy `build/raylib/libraylib.a` into `lib/` and `src/raylib.h`, `src/raymath.h`,
   `src/rlgl.h` into `include/`.

The patched library is harmless on machines where DirectInput works: it only
changes what happens when it fails.
