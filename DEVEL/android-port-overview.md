# NetHack 3.6.7 Android Port Analysis

This document describes the architectural integration of the Android port within the NetHack 3.6.7 codebase.

## 1. Architectural Overview
The Android port follows a "thick native, thin wrapper" model. The core NetHack engine (version 3.6.7) is compiled as a native shared library (`libnethack.so`), which is then managed by a Java-based Android application. The UI is implemented by delegating NetHack's windowing system calls to the Java layer via JNI.

## 2. Overlay Structure
The port is implemented as an "overlay" on the vanilla NetHack source. The primary additions are located in the `sys/android/` directory, while standard NetHack configuration files are modified to recognize the Android platform.

### Key Directories and Files
*   **`sys/android/`**: The heart of the port.
    *   `winandroid.c`: Implements the NetHack `window_procs` interface via JNI callbacks.
    *   `androidmain.c`: Contains the `NetHackMain` entry point and Android-specific lifecycle logic.
    *   `androidunix.c`: Android-specific Unix-like system glue.
    *   `setup.sh`: A script that prepares the repository for building by copying Android-specific Makefiles.
    *   `app/`: A standard Gradle-based Android project.
*   **`include/androidconf.h`**: Contains Android-specific engine definitions (e.g., file system behavior, graphics options).
*   **`include/global.h`**: Modified to include `androidconf.h` when the `ANDROID` macro is defined.

## 3. Porting Layers

### Native Layer (C)
The native layer is responsible for running the NetHack game logic.
*   **Windowing Interface**: `winandroid.c` implements the `and_procs` (of type `struct window_procs`). Every NetHack UI action (like `putstr`, `print_glyph`, or `select_menu`) is converted into a JNI call to the Java UI layer.
*   **Execution Control**: `androidmain.c` uses `setjmp`/`longjmp` to wrap the engine's execution. This allows the engine to "exit" back to the Android activity without terminating the entire process, which is necessary for a shared library environment.
*   **Configuration**: `androidconf.h` disables features that don't make sense on Android (like shell escapes) and enables others (like `ASCIIGRAPH` and `SELECTSAVED`).

### Android App Layer (Java/Kotlin)
The Android app resides in `sys/android/app/`.
*   **ForkFront Library**: The port leverages an external library (`com.tbd.forkfront:lib`) that provides the main `ForkFront` Activity. This library handles the heavy lifting of the Android UI, including terminal emulation, tileset rendering, and input handling.
*   **JNI Entry Points**: The native code defines JNI entry points (e.g., `Java_com_tbd_forkfront_NetHackIO_RunNetHack`) that the Java layer calls to boot the engine.

## 4. Build Pipeline
The build process integrates traditional NetHack Makefiles with the Android NDK and Gradle:

1.  **Preparation**: `sys/android/setup.sh` is executed to copy platform-specific Makefiles (`Makefile.src`, `Makefile.top`, etc.) into the source tree.
2.  **Native Build**: A standard `make` command, using the NDK toolchain, compiles the C source code into `libnethack.so`. The output is placed directly into the Android app's `libs/` directory.
3.  **Data Compilation**: The `make install` target compiles NetHack's data files (dungeons, rumors, etc.) and installs them into the Android app's `assets/nethackdir`.
4.  **APK Packaging**: Gradle is used to compile the Android project and package the native library and assets into the final APK.

## 5. Summary of Porting Strategy
The port avoids invasive changes to the core NetHack logic by concentrating platform-specific code in the `sys/android` directory and using the established `window_procs` abstraction. By treating the Android UI as a separate entity communicating via JNI, the port maintains high compatibility with the 3.6.7 codebase while providing a native Android experience.
