This project is a modernized Android port of NetHack 3.6.7, following a "thick native, thin wrapper" architectural model.

# Core Design Principles

When making architectural or UI decisions, adhere to these guiding principles:

- **Touch & Gamepad First**: The primary input methods are the touchscreen and physical gamepads. The soft keyboard should **never** be required for gameplay, only for text/numeric entry.
- **Native Android Experience**: Avoid "PC-to-Android" ports that rely on virtual cursors or keyboard emulation. The UI should feel native, using Android-appropriate gestures, haptics, and layout patterns (inspired by the philosophy of NetHackDS).
- **Full screen and immersive**: The original version of those port made these features optional, but this is a game. Fullscreen and immersive just makes sense.
- **Radical Configurability**: NetHack's complexity is extreme. The interface must be highly flexible and configurable to accommodate diverse playstyles rather than enforcing a single "correct" control scheme.
- **Modern Performance**: Target modern Android features (like 16KB page alignment) while maintaining the core stability of the NetHack 3.6.7 engine.

# Architectural Quick Reference

The port uses a **Clean Overlay** strategy. The core NetHack source tree remains vanilla, while all Android-specific logic is isolated in `sys/android/`.

- **Native Engine (C)**: Located in `src/` and `include/`. Compiled into `libnethack.so`.
- **JNI Bridge**: `sys/android/winandroid.c` implements the NetHack `window_procs` interface (`and_procs`) to communicate with the Java layer.
- **Entry Point**: `sys/android/androidmain.c` contains `NetHackMain` and uses `setjmp`/`longjmp` for lifecycle management.
- **Java UI Layer**: Located in `sys/android/forkfront/` (a Git submodule). This library handles rendering, menus, and input.
- **Android App**: Located in `sys/android/app/`. This is the Gradle project that packages the engine and UI.

# Technical Standards & Style

- **C Coding Style**:
    - **Indentation**: 4 spaces, **no tabs**.
    - **K&R Style**: Function arguments must be declared K&R style (return type, name, arguments on separate lines, types before the opening brace).
    - **Line Limit**: Max 78 characters.
    - See `DEVEL/code_style.txt` for exhaustive details.
- **Java Coding Style**:
    - Standard Android conventions, but **Java only**.
    - Maintain the legacy package structure in `sys/android/forkfront/lib/src`.

# Component Mapping

| Area | Native (C) | Bridge (Java) | UI Implementation (Java) |
| :--- | :--- | :--- | :--- |
| **Map/Tiles** | `src/display.c` | `NetHackIO.java` | `NHW_Map.java`, `Tileset.java` |
| **Messages** | `src/pline.c` | `NetHackIO.java` | `NHW_Message.java` |
| **Stats** | `src/botl.c" | `NetHackIO.java` | `NHW_Status.java` |
| **Menus** | `src/window.c` | `NetHackIO.java` | `NHW_Menu.java`, `NH_Dialog.java` |
| **Gamepad** | N/A | `NetHackIO.java` | `gamepad/GamepadDispatcher.java` |
| **Coordination**| `androidmain.c` | `NetHackIO.java` | `NH_State.java` (The "Brain") |

# Workflow: Build, Install, and Run

## Building
The build system is a hybrid of traditional Makefiles (for native code) and Gradle.
1. **Navigate to the Android directory**: `cd sys/android`
2. **Execute Build**: `./gradlew assembleDebug`
   - *Note*: This automatically handles `setup.sh`, native compilation via NDK, and data file compilation.
   - **Efficiency**: Avoid full rebuilds. Rely on incremental builds to save time.

## Installation
The resulting APK is located at:
`sys/android/app/build/outputs/apk/debug/app-debug.apk`

To install via CLI (assuming a device/emulator is connected):
```bash
adb install -r sys/android/app/build/outputs/apk/debug/app-debug.apk
```

## Running & Testing
To launch the application on a connected device:
```bash
adb shell am start -n com.tbd.NetHack/com.tbd.forkfront.ForkFront
```

- **Emulators**: Use `x86_64` images for best performance.
- **Real Hardware**: The application can take significant time to initialize on the first run.
- **Manual Interaction**: When testing on real hardware, you must provide clear instructions for the human operator and wait for their feedback.

# Working with the Frontend (ForkFront)

Requests involving the **"frontend"**, **"UI"**, or **"user interface"** refer to the `forkfront` submodule located at `sys/android/forkfront/`.

- **Independent Management**: This is a standalone Git submodule. Changes to the UI must be committed within this submodule specifically.
- **Git Operations**: Always use the `-C` flag for git operations in this directory (e.g., `git -C sys/android/forkfront commit ...`).
- **Composite Build**: The main Android build automatically includes local changes from this submodule via Gradle.

# Critical Constraints & Guidelines

- **Language**: This is a **Java-only** project. **NEVER** introduce Kotlin or suggest a migration.
- **Submodules**: **ALWAYS** use `git -C` for any operation targeting `sys/android/forkfront/`.
- **UI Modifications**: 
  - To add engine-to-UI calls: Modify `winandroid.c` (C), `NetHackIO.java` (Java), and `NH_Handler.java` (Interface).
  - To change UI behavior: Focus on `sys/android/forkfront/` and `NH_State.java`.
- **Environment**: Requires JDK 17, Android SDK 34, and NDK `21.4.7075529`.

# Documentation Reference

For deeper dives, see:
- [Port Overview](DEVEL/android-port-overview.md): Strategy and directory structure.
- [Architecture](DEVEL/port-architecture.md): Detailed data flow and component layers.
- [Build Guide](DEVEL/android-port-build.md): Full prerequisites and troubleshooting.
