# Zookeeper DX — Nintendo Switch port (Unity / IL2CPP wrapper)

This is a native wrapper / loader that runs the original ARM64 build of **Zookeeper DX**
(`jp.kiteretsu.zookeeper_dx`, Unity 2022.3.62f2, IL2CPP) on Switch homebrew. It contains no game
code and no game assets. It loads the game's own native libraries (`libil2cpp.so`, `libunity.so`,
`libmain.so`) and recreates, natively, the thin Android/JNI layer the engine expects — a fake
`JNIEnv`/`JavaVM`, a GLES3/EGL context, an OpenSL-ES audio device, touch/controller input,
`PlayerPrefs`-backed save data, and the handful of "Java wrapper" callbacks the engine makes. You
supply the libraries and the assets from a copy of the game you legally own.

## Install & run

You need files from `jp.kiteretsu.zookeeper_dx` (APK + its asset archives; versionCode `10007`).

Copy the `.nro` to your SD card and place the game files next to it, in the same folder:

```
sdmc:/switch/zookeeper/
├── zookeeper_nx.nro
├── libil2cpp.so                 <- from your APK: lib/arm64-v8a/
├── libunity.so                  <- from your APK: lib/arm64-v8a/ 
├── libmain.so                   <- from your APK: lib/arm64-v8a/
└── assets/                      <- the whole assets/ folder from the APK
    └── bin/Data/ ... (data.unity3d, global-metadata.dat, etc.)
```
The wrapper runs under Atmosphère via **title override** (launch the installed game while holding a
chosen button). The game folder is `sdmc:/switch/zookeeper/`.

Optionally, drop a `cursor.png` (64x64, transparency supported) in the same folder to replace the on-screen cursor with your own.

## Controls
 
| Input | Action |
| --- | --- |
| `+` | Toggle the on-screen cursor |
| `-` | Toggle gyro pointing (tilt/turn the controller to aim) |
| Left stick | Move the cursor |
| `L` / `R` | Recenter the cursor to the middle of the screen (helps gyro aiming) |
| `A` / `ZR` / `ZL` | Tap / confirm (ZL and ZR let you play one-handed) |
| `B` | Back button
| D-pad up / down | Adjust sensitivity of whatever is driving the cursor |

A USB mouse works in both handheld and docked: move to control the cursor, left-click to tap, and use the scroll wheel to change 
sensitivity.
Your stick, mouse and gyro sensitivities are remembered in `pointer.cfg` automatically after in-game adjustment.

## Configuration

On first launch the wrapper writes a documented `sdmc:/switch/zookeeper/config.txt`. Options are
`name value` lines (whitespace-separated); `#` starts a comment.

* `portrait` — the render is rotated 90° to fill the screen (hold the console rotated to play):
  `1` (default) rotates clockwise (right Joy-Con up); `2` rotates the other way (left Joy-Con up);
  `0` disables rotation (stretched 16:9). Set `portrait 0` if you play docked on a TV and don't
  want a sideways picture.

Save data (settings, progress, high scores) is stored in
`sdmc:/switch/zookeeper/prefs.kv` and persists across launches.

## Requirements (to build)

Install devkitPro with the Switch toolchain and these packages:

```
pacman -S switch-dev
pacman -S switch-mesa switch-libdrm_nouveau switch-sdl2 switch-freetype switch-libpng switch-zlib switch-bzip2
```

`switch-mesa` provides GLES3/EGL; `switch-sdl2` backs the window/HID/audio device; `switch-zlib`
is used for the save/compression paths. Then run `make` in the project root to produce
`zookeeper_nx.nro`.

## Credits

Special thanks to Naga for helping with Unity.

The loader/shim infrastructure (`so_util`, `libc_shim`, `util`, `error`) derives from the
open-source Switch `.so` loader lineage by Andy Nguyen, fgsfds and contributors, which builds on
TheOfficialFloW's Vita/Switch loader work — all MIT-licensed (see `LICENSE`). The Unity/IL2CPP JNI
layer, the GLES/EGL and audio bridges, the PlayerPrefs save backend, input, TATE rotation, the
boot-time `libunity` patcher, and the main loop in this project are new. Thanks to everyone in that
lineage for making this approach possible.

This project ships no game code or assets. Zookeeper DX is © KITERETSU Inc.; provide your own
legally obtained copy.
