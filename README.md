# NetHack Android DS

A modernized Android port of [NetHack 3.6.7](https://www.nethack.org/), built for touchscreens and
physical gamepads. This is a personal, in-development project — expect rough edges, but also expect
it to keep getting better.

Releases (APK downloads) are posted on the
[GitHub releases page](https://github.com/fancypantalons/NetHack-Android-DS/releases).

**Requires Android 13 or later.**

## Design Philosophy

This port is built around a few firm ideas:

- **Touch and gamepad first.** A physical keyboard or mouse should never be required for gameplay.
  Navigation, commands, and menus all work through touch gestures or a physical gamepad. The soft
  keyboard only appears for text entry.
- **Native Android experience.** This port tries to create a gaming experience that feels natural and not like a soft keyboard bolted onto a terminal-native game. In this way it borrows a lot from my work on
  [NetHackDS](https://github.com/fancypantalons/NetHack).
- **Radical configurability.** NetHack is a deep game. The interface must accommodate diverse
  playstyles rather than enforcing a single correct way to play.
- **Always fullscreen and immersive.** It's a game. There's no good reason to give up screen real
  estate.

## Showcase

### Portrait — Touch Mode

Like the [original project this is based on](https://github.com/gurrhack/NetHack-Android/), this port supports tapping to control as well as a widget overlay system allowing for deep customization:

https://github.com/user-attachments/assets/64e5a21e-5a46-4c18-8045-59dd401f6e3b

### Landscape — Touch Mode

Orientation-specific layouts allow for unique portrait and landscape layouts.

https://github.com/user-attachments/assets/24721591-48c7-439e-9c49-92b2c60238ce

### Dual Screen — Gamepad

In truth the reason this port exists is because I wanted something I could play on my Ayn Thor, so this port includes first-class support for dual-screen devices and gamepads:

https://github.com/user-attachments/assets/9ed3e8fd-cebb-450d-806b-4d1c1f3b3503

### Overlay Editor

The overlay system is designed to allow the user to relocate nearly any on-screen element, allowing for deep customization:

https://github.com/user-attachments/assets/9ae8ba2b-7f65-4efd-8f7e-309b227151b7

---

## Architecture

The port follows a **thick native, thin wrapper** model:

- The vanilla NetHack 3.6.7 engine compiles to a native shared library (`libnethack.so`) via the
  Android NDK. The core game logic is untouched.
- A thin JNI bridge (`sys/android/winandroid.c`) connects the engine to the Java UI layer by
  implementing NetHack's standard `window_procs` interface.
- The UI is implemented by [ForkFront-DS](https://github.com/fancypantalons/ForkFront-Android-DS), a dedicated Android
  library that lives in `sys/android/forkfront/` as a Git submodule. It handles rendering, input,
  menus, dialogs, and settings.

For a deeper dive, see the docs in [`DEVEL/`](DEVEL/):

- [Port Overview](DEVEL/android-port-overview.md) — strategy and directory structure
- [Architecture](DEVEL/port-architecture.md) — data flow and component layers
- [Build Guide](DEVEL/android-port-build.md) — full prerequisites and troubleshooting

---

## Building from Source

Requirements: JDK 17, Android SDK 34, NDK `21.4.7075529`.

```bash
cd sys/android
./gradlew assembleDebug
```

The resulting APK is at `sys/android/app/build/outputs/apk/debug/app-debug.apk`.

See [`DEVEL/android-port-build.md`](DEVEL/android-port-build.md) for full setup instructions.

---

## Origins

This project is a hard fork of [NetHack-Android](https://github.com/gurrhack/NetHack-Android) by
gurrhack, which provided the original native bridge and build system for NetHack 3.6.7 on Android.
The UI library, [ForkFront](sys/android/forkfront/README.md), is similarly a hard fork of gurrhack's
[ForkFront-Android](https://github.com/gurrhack/ForkFront-Android). Both forks diverge substantially
in architecture and design philosophy, and while it could not exist without all that previous hard work, I've opted to deviate pretty substantially without a focus on contributing back to the original (now fairly quiet) project.

NetHack itself is the work of the [NetHack Dev Team](https://www.nethack.org/) and a long line of
contributors stretching back to 1987. See [`README.NetHack`](README.NetHack) for the original
upstream README.

---

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md). The [ForkFront submodule](sys/android/forkfront/) has its
own [CONTRIBUTING.md](sys/android/forkfront/CONTRIBUTING.md) with the same conventions.
