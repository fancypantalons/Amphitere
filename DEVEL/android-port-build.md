# Building the Amphitere Android Port (NetHack 5.0.0)

## Prerequisites

- **Linux** (64-bit; instructions verified on Debian/Ubuntu)
- **JDK 17**
- **Android SDK** with `platforms;android-34` and `build-tools;35.0.0`
- **Android NDK** `21.4.7075529` (via `sdkmanager`: `ndk;21.4.7075529`)
- **Host build tools**: `gcc`, `make`, `bison`, `flex`

## One-time setup

These steps are required once per fresh checkout, and again any time you switch from another branch (e.g. the 3.6.7 port) which may have left stale Makefiles behind.

### 1. Configure the NDK

Create or update `sys/android/local.properties`:

```
sdk.dir=/path/to/Android/Sdk
ndk.dir=/path/to/Android/Sdk/ndk/21.4.7075529
```

Alternatively, export `ANDROID_NDK_ROOT` before building. The build will fail with a clear error if neither is set.

To target a specific ABI or API level (optional; defaults shown):

```
android.abi=arm64-v8a
android.platform=21
```

### 2. Clear stale Makefiles (required when switching branches)

```bash
rm -f Makefile src/Makefile dat/Makefile util/Makefile doc/Makefile
```

### 3. Generate the build system

From the repo root:

```bash
sh sys/unix/setup.sh sys/unix/hints/android.500
```

### 4. Fetch Lua

From the repo root:

```bash
make fetch-lua
```

This downloads and does an initial host compile of Lua 5.4. The Gradle build will wipe the host objects and recompile Lua for the Android target automatically.

## Building

From `sys/android/`:

```bash
./gradlew assembleDebug       # build only
./gradlew installDebug        # build and install to a connected device
```

The Gradle build orchestrates the full pipeline:
1. Runs `setup.sh` if `src/Makefile` is absent (skipped on incremental builds)
2. Builds host-side tools (`makedefs`, `dlb`, `tilemap`) with host `gcc`
3. Generates `src/tile.c` (the glyph→tile index table) via `tilemap`
4. Generates and packs game data into `dat/nhdat` via `dlb`
5. Wipes host Lua objects and recompiles Lua for the Android target with the NDK clang
6. Cross-compiles `libnethack.so` for `arm64-v8a` (or the configured ABI)
7. Copies the `.so` and data assets into the APK

Output APK: `sys/android/app/build/outputs/apk/debug/app-debug.apk`

## Troubleshooting

**NDK not found**: Set `ndk.dir` in `local.properties` or export `ANDROID_NDK_ROOT`.

**Makefile errors after a branch switch**: You likely have stale Makefiles from the other port. Delete them (step 2 above) and re-run setup.

**Lua linker error** (`File in wrong format` / `EM: 62`): The host-compiled Lua objects were not cleaned before the Android link step. Run `make clean` from the repo root, then rebuild.

**`makedefs` build errors**: Ensure `gcc` and `gcc-multilib` are installed for the host build tools step.
