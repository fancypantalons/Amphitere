# Building the NetHack Android Port

This document outlines the requirements and steps to build the modernized Android port of NetHack 3.6.7.

## 1. Prerequisites

### Software Dependencies
*   **Operating System**: Linux (instructions verified on 64-bit Ubuntu).
*   **JDK 17**: Required by the modernized Gradle build system.
*   **Android SDK**: Command-line tools and platforms.
*   **Android NDK**: Version `21.4.7075529`.
*   **Native Build Tools**: `gcc`, `make`, `bison`, `flex`, and `gcc-multilib`.

### Environment Variables
*   `ANDROID_HOME` or `ANDROID_SDK_ROOT`: Should point to your Android SDK installation.
*   `ANDROID_NDK_ROOT`: (Optional) Points to your NDK if not in the default SDK location.

## 2. Preparation Steps

### Install Android Build Tools
Ensure the following components are installed via `sdkmanager`:
*   `platforms;android-34`
*   `ndk;21.4.7075529`

### Configuring the NDK Path
The build system automatically looks for the NDK in standard locations. If your NDK is installed in a non-standard path, set the `ANDROID_NDK_ROOT` environment variable or update the `NDK` variable in `sys/android/Makefile.src`.

## 3. Build Process

The build process is now fully automated. The native NetHack engine and data files are automatically compiled and packaged as part of the Gradle build.

1.  **Navigate to the Android project directory**:
    ```bash
    cd sys/android
    ```

2.  **Build the APK**:
    ```bash
    ./gradlew assembleDebug
    ```
    *This single command will:*
    *   Initialize the NetHack build environment (`setup.sh`).
    *   Compile the native engine for all supported ABIs (`arm64-v8a`, `armeabi-v7a`, `x86_64`).
    *   Compile the game data (dungeons, rumors, etc.).
    *   Package everything into a debug APK.

3.  **Output Locations**:
    The resulting APKs are located in:
    *   Debug: `sys/android/app/build/outputs/apk/debug/app-debug.apk`
    *   Release: `sys/android/app/build/outputs/apk/release/app-release-unsigned.apk`

## 4. Supported Architectures
The build generates shared libraries for the following ABIs by default:
*   `arm64-v8a` (Modern 64-bit ARM devices)
*   `armeabi-v7a` (Older 32-bit ARM devices)
*   `x86_64` (Modern Android Emulators)

To modify the target ABIs, edit the `ALL_ABIS` variable in `sys/android/Makefile.src`.

## 5. Deployment
1.  Locate the APK in `sys/android/app/build/outputs/apk/debug`.
2.  Copy the APK file to your device or emulator.
3.  Install and run.

## 6. Troubleshooting
*   **NDK Errors**: If the build fails with "compiler not found", ensure NDK `21.4.7075529` is installed and the path in `sys/android/Makefile.src` is correct.
*   **Java Errors**: Ensure `JAVA_HOME` points to JDK 17 or higher.
*   **Makedefs Conflicts**: If `makedefs` fails to build on the host, check `include/system.h` for conflicting function prototypes (e.g., `sleep`, `srand48`).
