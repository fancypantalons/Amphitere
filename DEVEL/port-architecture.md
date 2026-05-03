# NetHack Android Port Architecture

This document provides a detailed overview of the NetHack Android port's architecture, designed to guide developers and AI agents in navigating and modifying the codebase.

## 1. Architectural Philosophy: The Clean Overlay
The port follows a **Clean Overlay** strategy. This means:
*   The core NetHack source tree (`src/`, `include/`, `dat/`, etc.) remains identical to the vanilla 3.6.7 release.
*   All Android-specific logic is contained within the `sys/android/` directory.
*   The port interacts with the engine via the standard NetHack `window_procs` interface, avoiding invasive changes to the game logic.

## 2. Repository Structure
The project is split across two primary components:

### A. NetHack-Android (The Host)
*   **Role**: Contains the NetHack engine, the native bridge, and the Android application shell.
*   **Key Locations**:
    *   `src/`, `include/`: Vanilla NetHack engine.
    *   `sys/android/`: Native bridge (`winandroid.c`), Android main entry (`androidmain.c`), and build logic (`Makefile.*`).
    *   `sys/android/app/`: The Android Gradle module, manifest, and resources.

### B. ForkFront-Android (The UI Library)
*   **Role**: A dedicated Android library that implements the actual User Interface (Map rendering, Menus, Input handling).
*   **Integration**: Managed as a **Git Submodule** in `sys/android/forkfront/`.
*   **Local Development**: Integrated via Gradle **Composite Build** (`includeBuild`). Any changes made in `sys/android/forkfront/` will be picked up during the main APK build.
*   **Key Package**: `com.tbd.forkfront`

## 3. Component Layers & Data Flow

### Layer 1: The Native Engine (C)
The game logic runs in a native background thread. It knows nothing about Android; it only knows it is calling functions in a `window_procs` struct named `and_procs`.

### Layer 2: The JNI Bridge (`winandroid.c`)
This C file implements the `and_procs` interface.
*   **Initialization**: `Java_com_tbd_forkfront_NetHackIO_RunNetHack` caches JNI Method IDs for Java callbacks.
*   **Event Translation**: When the engine calls `and_putstr(wid, attr, text)`, the bridge calls the corresponding Java method `putString` on the `NetHackIO` instance.

### Layer 3: The IO & State Management (Java)
Located in `com.tbd.forkfront`:
*   **`NetHackIO.java`**: The low-level JNI interface. It spawns the `nh_thread` and hosts the callback methods invoked by the C bridge.
*   **`NH_Handler.java`**: An interface defining the callbacks from the engine.
*   **`NH_State.java`**: The "Brain" of the UI. It implements `NH_Handler` and coordinates between the game engine's requests and the various UI components.

### Layer 4: The UI Display (Java/Android)
*   **`NHW_Map.java`**: Handles tile-based rendering of the game map.
*   **`NHW_Message.java`**: Manages the message log/top line.
*   **`NHW_Status.java`**: Displays player stats (HP, AC, Gold, etc.).
*   **`NHW_Menu.java` / `NH_Dialog.java`**: Handles NetHack's various menu and text windows using Android Fragments/Dialogs.
*   **`Input.java` / `SoftKeyboard.java`**: Manages touch events, gestures, and the virtual keyboard.

## 4. Execution Lifecycle
1.  **Launch**: `AndroidManifest.xml` starts `com.tbd.forkfront.ForkFront` (the main Activity).
2.  **Startup**: `ForkFront` creates an `NH_State` instance.
3.  **Engine Start**: `NH_State` tells `NetHackIO` to `start()`.
4.  **Native Boot**: `NetHackIO` spawns a thread that calls the native `RunNetHack`.
5.  **Game Loop**: The engine runs its loop, sending UI updates back through the JNI bridge to `NH_State`, which updates the Android Views.

## 5. Build System
The build is a hybrid process managed by Gradle:
1.  **Native Setup**: Gradle invokes `sys/android/setup.sh` to configure the source tree.
2.  **Utility Compilation**: `Makefile.utl` builds host-side tools (`makedefs`, etc.).
3.  **Data Compilation**: `Makefile.dat` uses the host tools to compile dungeons and data into `assets/nethackdir/`.
4.  **Native Library**: `Makefile.src` compiles the C engine and bridge into `libnethack.so` using the Android NDK.
5.  **APK Packaging**: Gradle bundles the assets, the native library, and the Java code into the final APK.

## 6. Key Files for Modification
*   **Adding a JNI Callback**: Modify `winandroid.c` (C), `NetHackIO.java` (Java), and `NH_Handler.java` (Interface).
*   **Changing UI Layout**: Modify `ForkFront-Android`'s layout XMLs and `NH_State.java`.
*   **Updating Engine Logic**: Modify the core C files in `src/` (standard NetHack development).
*   **Adjusting Build Flags**: Modify `sys/android/Makefile.src`.
