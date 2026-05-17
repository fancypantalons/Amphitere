# Building the Amphitere Android Port (NetHack 3.6.7)

## Prerequisites

- **Linux** (64-bit; instructions verified on Debian/Ubuntu)
- **JDK 17**
- **Android SDK** with `platforms;android-34` and `build-tools;35.0.0`
- **Android NDK** `21.4.7075529` (via `sdkmanager`: `ndk;21.4.7075529`)
- **Host build tools**: `gcc`, `make`, `bison`, `flex`

## One-time setup

### 1. Configure the NDK

Create or update `sys/android/local.properties`:

```
sdk.dir=/path/to/Android/Sdk
ndk.dir=/path/to/Android/Sdk/ndk/21.4.7075529
```

Alternatively, export `ANDROID_NDK_ROOT` before building. The build will fail with a clear error if neither is set.

To target a specific ABI (optional; default shown):

```
android.abi=arm64-v8a
```

### 2. Clear stale Makefiles (required when switching from the 5.0.0 branch)

The 5.0.0 port generates Makefiles via `sys/unix/setup.sh` with Android-specific hint files. These are incompatible with the 3.6.7 Makefile templates in `sys/android/`. If you have switched from that branch, delete the stale files first:

```bash
rm -f Makefile src/Makefile dat/Makefile util/Makefile doc/Makefile
```

The Gradle build will then run `sys/android/setup.sh` automatically on the first build to install the correct Makefiles.

## Building

From `sys/android/`:

```bash
./gradlew assembleDebug       # build only
./gradlew installDebug        # build and install to a connected device
```

The Gradle build handles the full pipeline:
1. Runs `sys/android/setup.sh` if the repo root `Makefile` is absent, installing the 3.6.7 Android Makefile templates
2. Runs `make android-all` to compile the native engine and package game data into `libnethack.so`

Output APK: `sys/android/app/build/outputs/apk/debug/app-debug.apk`

## Troubleshooting

**NDK not found**: Set `ndk.dir` in `local.properties` or export `ANDROID_NDK_ROOT`.

**Makefile errors after switching from the 5.0.0 branch**: Delete the stale Makefiles (step 2 above) and rebuild — Gradle will reinstall the correct ones automatically.

**`makedefs` or compiler errors**: Ensure `gcc` and `gcc-multilib` are installed.
