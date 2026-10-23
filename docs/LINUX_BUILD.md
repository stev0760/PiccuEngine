# PiccuEngine: a native x86_64 Linux build

Built 2026-10-08 for Matcen's client-compatibility pass (REL12). As far as we know this is the first native Linux build
of PiccuEngine: upstream Piccu builds only on Windows, and its old Linux CMake path (32-bit, SDL 1.2, Loki-era
directories that no longer exist) is dead. This build reuses the SDL3 path that Piccu's Windows build takes with
`USE_SDL`, and adds the Linux pieces that path was missing.

**Status:** it compiles and links with no errors: the engine, the Direct TCP/IP connection module, all nine netgames,
the 56 level and object scripts and a Piccu `d3-linux.hog`. A headless start (SDL offscreen driver, no window) loads
the data, initialises OpenGL and OpenAL, and reaches the pilot dialog ("Please enter a name for your pilot."), twice
running. In a window on X11 it reaches the menus, joins a Matcen server over Direct TCP/IP, flies, chats and follows
a level change (the REL12 run, 2026-10-08; see "What runs"). Three things had to be fixed after the first windowed
start: the engine reads Linux arguments GNU-style (`--name`), the command-line join loads the connector by another
name, and `hogdir` wrote unsorted HOGs.

## Source and toolchain

- PiccuEngine `56cab84` (2025-10-14, "modify ScaleSDLAxisReading to ensure it won't div by 0..."), the checkout at
  `/home/steve/Projects/mine/PiccuEngine`, unmodified. The build works on a copy of the tree with the patch applied.
- Arch Linux, kernel 7.2 (zen), GCC 16.2.1, CMake 4.4.3, Ninja.
- System packages, no vcpkg: `sdl3` 3.4.16, `openal` 1.25.2 (OpenAL Soft), `mesa` (libOpenGL/libGLX through GLVND).
  Piccu's `vcpkg.json` asks only for `sdl3`; the system SDL3 serves.

## Build

```sh
cd /home/steve/Projects/mine/PiccuEngine
mkdir -p build/linux-native/src
git archive 56cab84 | tar -x -C build/linux-native/src
cd build/linux-native/src
git init -q && git add -A && git commit -qm "pristine 56cab84"    # only so that git apply can create the symlinks
git apply ../LINUX_BUILD.patch
cmake -S . -B ../obj -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo
ninja -C ../obj                                                    # about 15 s on 20 cores
```

`build/` is in Piccu's `.gitignore`, so none of this shows in the checkout's `git status`. The patch also applies to
the checkout itself (`git apply --check build/linux-native/LINUX_BUILD.patch` passes). Building in the tree would
then work from any build directory, but this build was made from the copy so that the checkout stays clean.
`RelWithDebInfo` defines `RELEASE` the way Piccu's Release build does, and it keeps symbols for a backtrace.

Outputs, in `build/linux-native/obj/`:

| File | What it is |
|---|---|
| `PiccuEngine` | the engine (ELF x86_64, PIE, linked `-rdynamic`) |
| `piccuengine.hog` | Piccu's shaders and its own strings. The engine looks for it in the **working directory** at start-up |
| `online/Direct TCP~IP.piccucon` | the Direct TCP/IP connection module, a bare `.so`. Piccu's loader accepts either a HOG holding `Direct TCP~IP.so` or the bare library |
| `online/Tracker.piccucon` | the tracker connection module (not tested) |
| `netgames/*.d3m` | `anarchy`, `co-op`, `ctf`, `entropy`, `hoard`, `hyper-anarchy`, `monsterball`, `robo-anarchy`, `team anarchy`: bare `.so` files with DMFC linked in |
| `d3-linux.hog` | the 56 scripts as `.so`, plus Piccu's string tables (`scripts/data/linuxfullhog`) |

## Running

`make-run-dir.sh <run dir> <data dir>` puts the engine, `piccuengine.hog`, `d3-linux.hog`, the connection module and
the netgames in a run directory, and symlinks `d3.hog`, `extra.hog`, `extra13.hog` and `merc.hog` from a Descent 3 data
directory. `build/linux-native/run/` is one such directory, linked to the Matcen soak lab's HOGs. Put the server's
mission `.mn3` in `missions/`.

```sh
cd build/linux-native/run
XDG_DATA_HOME=/some/scratch/xdg ./PiccuEngine --setdir "$PWD" --tempdir "$PWD/tmp" --windowed --nosound --nomusic \
    --pilot <name> --useport <free port> --directip <server ip>:<port>
```

- **Two dashes.** On Linux, `FindArg` (`Descent3/args.cpp`, Loki code) turns `-name` into `--name` before it
  compares, so every switch must be typed with two dashes. A one-dash switch is silently ignored: `-nosound` does not
  silence the game, `-pilot` does not pick the pilot, `-directip` does not join. Single-letter forms such as `-w` and
  `-m` are matched as written by `FindArgChar`.

- **Data:** the base directory is `--setdir` (default: the working directory). `piccuengine.hog` is read from the
  working directory, so launch from inside the run directory.
- **Pilots and settings:** with `--setdir`, Piccu's user directory is the base directory, so pilots (`*.plt`) and
  `custom/` land in the run directory. The settings file (`.PiccuEngineRegistry`) goes to the SDL pref path,
  `$XDG_DATA_HOME/PiccuEngine/`, and `$XDG_DATA_HOME/Piccu Engine/` is the user directory without `--setdir` (where
  the REL12 run's first pilot landed, while its one-dash `-setdir` was ignored). Set `XDG_DATA_HOME` to keep a test
  away from a real install.
- **First start:** a pilot named by `--pilot` that does not exist opens the "enter a name" dialog. The name field
  takes SDL text input: keys sent with `xdotool key --window` (X `SendEvent`) reach the menus and the in-game chat but
  not that field, so an agent-made pilot came out nameless and had its name written into the file instead. Creating a fresh
  pilot in Piccu is safer than copying one from another engine. The `.plt` version (0x2B) is the same, but the
  controls block depends on the engine's controller-function table.
- **Joining:** `--directip ip:port` connects on its own once the connector is also reachable as
  `online/TCP-IP.piccucon`: `AutoConnectLANIP` (`Descent3/menu.cpp`) loads the connector named `TCP-IP`, while the
  lanclient CMake names its output `Direct TCP~IP.piccucon` (the name the Multiplayer menu lists). `make-run-dir.sh`
  adds the link. The same mismatch is in Piccu's own tree, so a Windows build would need the same link unless its
  package renames the file. `+connect ip:port` does nothing unless `+cl_pxotrack 0` and `--directip` are given too
  (the same code as upstream). When the server runs on the same machine, pass `--useport` with a different port, so
  that the client does not try to bind the server's game port.
- The window opens at 640x480. `--nosound` silences the game, although OpenAL still opens a device.
- **Mouse.** In flight the engine turns on SDL relative mouse mode, which on X11 confines the pointer to a focused
  window; the SDL path has no `-nomousegrab`. The REL12 run kept the operator's pointer free with an `LD_PRELOAD` shim
  that made `SDL_SetWindowRelativeMouseMode` a no-op, so mouse flight was not tested.

## The patch, file by file (`LINUX_BUILD.patch`, 30 files, +354/-87)

Every change is guarded by `__LINUX__`, `UNIX AND NOT APPLE` or `WIN32` or is portable, so the Windows build should be
unaffected. The Windows build was not tried.

**Build system**
- `CMakeLists.txt`: the `UNIX` block is replaced. It now takes the SDL3 path (`ddio_sdl`, `sdl`, `dd_sndlib` with
  OpenAL) and finds `SDL3`, `OpenAL` and `OpenGL`. It defines `__LINUX__ LINUX UNIX SDL3 MONO _REENTRANT HAVEALLOCA_H
  _MAX_PATH=260 _MAX_FNAME=256` and sets PIC, `-fpermissive -Wno-write-strings -Wno-multichar -fno-strict-aliasing`
  and `-rdynamic`. The connection module calls `mem_malloc_sub`, `mem_free_sub` and `Sleep` in the engine, which on
  Linux resolve only if the executable exports them.
- `CMakeLists.txt` also builds DMFC as a static library linked into each netgame, so that a `.d3m` is
  self-contained, and builds all nine netgames (their CMake was commented out). It gives them the names a server
  sends (`robo-anarchy`, `hyper-anarchy`, `team anarchy`, `co-op`), because the client loads whichever netgame the
  server names. Finally it builds the scripts with a Linux `CHECKSUM` (see LP64) and packs `d3-linux.hog` with
  Piccu's `hogdir`. The old scripts CMake needs a 32-bit `hogUtils-i686`, so it is not used.
- `tools/hogdir/hogdir.cpp`: sorts the entries case-insensitively before writing the header. The engine finds a
  file in a HOG by a binary search with `stricmp` (`cfile/CFILE.cpp`, `open_file_in_lib`), and on Linux
  `std::filesystem::directory_iterator` lists a directory in no particular order, so the first build's HOGs held
  files the engine could not find: `cfopen("lanclient.str")` failed, the connector's string table came back empty,
  and the Multiplayer menu fell straight back to the main menu. Windows lists NTFS directories sorted, which is why
  the Windows build never showed it.
- `Descent3/CMakeLists.txt`: on Linux it compiles `sdlmain.cpp` in place of the Loki `lnxmain.cpp`.
- `renderer/CMakeLists.txt`: `wgl.c` (the Windows GL loader) is compiled on Windows only.
- `sdl/CMakeLists.txt`: adds `sdl/sdllinux.cpp`.
- `lib/linux-case/`: symlinks `Application.h` and `inffile.h`. Two includes name a file whose case differs from
  the file on disk, and Windows does not care about case. This directory is on the Linux include path.

**New file `sdl/sdllinux.cpp`**: the Linux versions of what the Windows SDL build takes from `win32/`. These are
`GlobalAlloc/GlobalLock/GlobalFree`, `Sleep`, `itoa` and `strupr`; `Debug_Init`, `Debug_ErrorBox` and
`Debug_MessageBox` (they print to stderr); and the console that `-dedicated` uses (`con_init`, `con_defer`,
`con_Printf`, `con_Input`), which reads stdin without blocking.

**Source fixes**
- `ddio_sdl/sdlfile.cpp`, the SDL file layer, which had only ever run on Windows:
  - paths now use `/` (`ddio_MakePath` and `ddio_CleanPath` hard-coded `\`, which made a directory literally named
    `\custom`);
  - `ddio_SetWorkingDir` uses `chdir`;
  - `ddio_CopyFile` uses `std::filesystem`;
  - `ddio_GetFileSysRoots` returns `/`;
  - `ddio_GetUserDir` uses `$XDG_DATA_HOME` (or `~/.local/share`), because `SDL_GetUserFolder(SDL_FOLDER_SAVEDGAMES)`
    returns NULL on Linux and the engine then stopped with an error;
  - **`ddio_FindNextFile` gets its missing `return true`.** Without it, GCC's code jumped to address 0 in
    `DeleteTempFiles` on the second start. MSVC tolerated the missing return.
- `sdl/sdldatabase.cpp`: the settings file goes to the SDL pref path on Linux (the code had `#error` there). Spaces
  were also added between the string-literal macros, which C++11 requires.
- `lib/Macros.h`, `lib/linux/linux_fix.h`: in C++ on Linux, `min` and `max` are function templates instead of
  macros, because libstdc++ headers use `min()` and `max()` as names. `__min`, `__max`, `_min` and `_max` no longer
  depend on whether `Macros.h` was included. The templates return `std::common_type<A, B>::type` by value. The
  first version (builds up to 2026-10-23) returned `decltype((a < b) ? a : b)`, which for two arguments of one type
  is `T&`: a reference to the template's own by-value parameter, dangling by the time the caller reads it. See
  "Fixed fault" below.
- `lib/application.h`, `lib/appdatabase.h`: includes written with backslashes (`sdl\SDLApp.h`) now use `/`.
- `lib/networking.h`, `networking/networking.cpp`, `netcon/inetfile/{CFtp,Chttpget}.cpp`: SDL 1.2's `SDL.h` and
  `SDL_thread.h` become `<SDL3/SDL_thread.h>`, and `SDL_CreateThread(fn, data)` becomes SDL3's
  `(fn, name, data)`. `networking.h` also defines `closesocket` for Linux (used by `gamespy.cpp`).
- `netcon/includes/inetgetfile.h`: drops `<linux/ipx.h>`, which modern kernels no longer ship.
- `netcon/trackclient/trackclient.cpp`: `#include <thread>`.
- `netgames/dmfc/dmfcinternal.h`: `#include <ctype.h>`.
- `module/module.cpp`: on Linux, `dd_SplitPath` and `dd_MakePath` map to the `ddio_` functions. They existed only
  for Windows.
- `mve/mvelib.cpp`, `mve/mve_audio.cpp`: `_open`, `_read` and `_close` become the POSIX calls; `#include <string.h>`.
- `ddio_sdl/sdlio.cpp`, `sdl/sdlcontroller.cpp`: `dinput.h`, `windows.h` and `process.h` are included on Windows
  only.
- `ddio_sdl/sdlmouse.cpp`: `BOOL`/`TRUE`/`FALSE` become `bool`.
- `dd_sndlib/llsopenal.cpp`: `isnan<float>(x)` becomes `std::isnan(x)`.
- `Descent3/newrender.cpp`: two loops declared a second `element` in the scope of the range-for variable, and a
  temporary was bound to a non-const reference. MSVC accepts both; GCC does not. The behaviour is unchanged.
- `scripts/osiris_import.h`: the real `size_t` from `<stddef.h>`. The header typedef'd it as `unsigned int`, which
  is wrong on x86_64.

## LP64 (x86_64 Linux) notes

Piccu targets Windows, where `long` is 32 bits. On Linux x86_64 `long` is 64 bits, and the upstream
DescentDevelopers engine reworked these types; Piccu has not.

- **Script checksum.** A script loads only if its compiled `CHECKSUM` equals the engine's
  `Osiris_CreateGameChecksum()`, which adds up struct sizes. This build computes **2273889867**, read from the
  running engine. Upstream and Matcen x86_64 scripts are built for 2273889835, and Piccu's script CMake hard-codes
  the 32-bit 2273873307. So this build makes its own `d3-linux.hog`: **another engine's `d3-linux.hog` will not
  load its scripts in this engine.** `PICCU_OSIRIS_CHECKSUM` is a CMake cache variable, to be re-read if a struct
  changes.
- `pstypes.h` has `typedef unsigned long ulong`, so `object::flags` is 8 bytes here. Netgames and scripts built from
  this tree agree with the engine; a module built elsewhere would not.
- `lanclient.cpp` `AutoLoginAndJoinGame` copies `sizeof(unsigned long)` (8) bytes into the 6-byte
  `network_address.address`. The extra bytes overwrite `port`, which is set again on the next line, so it is
  harmless as written.
- GCC reports six int-to-pointer casts. The connection API passes ports as pointers (`api->vp[28]`, `vp[32]`) and
  casts them back, which works. `osiris_predefs.cpp` stores a script pointer through an `int`
  (`scripted_data_ptr = (void*)(*(int*)ptr)`), which truncates a 64-bit pointer; that path is single-player AI
  goals. The other casts are on the PXO tracker path.
- Not audited: every packet DMFC and the netgames serialise (they use explicit `int`, `short` and `float` packers,
  so a mismatch would show up as an LP64 bug against a Matcen or upstream x86_64 server, not as an ABI clash), the
  demo format, and saved games.

## What runs / what doesn't

Checked headlessly here, with `SDL_VIDEO_DRIVER=offscreen`, no display, under gdb:

- It starts, reads the HOGs and the table file, sets up the temporary and user directories, initialises GL (EGL
  offscreen) and OpenAL, and reaches the pilot-creation dialog, on two starts running.
- The game checksum is 2273889867, and the scripts in `d3-linux.hog` are built to match it.
- The game ignores SIGINT and SIGTERM in its menus (SDL turns them into a quit event that the menu loop does not
  act on). Stop it with SIGKILL or from its own menu.

Runtime with a real window (REL12, 2026-10-08): X11 on Cinnamon, a Matcen 0.10.4 Debug dedicated server
(`7b929e34`) on the same machine, 3v3 CTF on Bedlam (Polaris, then Apparition) with six bots, then Robo-Anarchy on
`d3.mn3` level 1 with four, the client driven with `xdotool` and captured with `import`:

- **Menus:** the pilot dialog, the pilot configuration, the main menu and the Multiplayer connection list work from
  the keyboard (Tab and the arrow keys move between options; Enter picks one).
- **Joining:** `--directip` joined the CTF and the Robo-Anarchy servers. The level scripts loaded (Polaris's
  "Magnetic polarity reversed!"), and the bots showed in the kill feed, the HUD team counts and the Robo-Anarchy
  player list. A level change (`$endlevel`) reloaded the next level and the client re-entered the game. Leaving from
  the Escape dialog was a clean leave on the server ("PiccuC has left the game").
- **Flight:** in about a minute of Robo-Anarchy every keyboard input (turn, thrust, pitch, respawn) moved the ship;
  with no input it stayed put.
- **Chat:** F8 sends a line; the bots' `!help`, `!follow`, `!status`, `!formup` replies and the Robo-Anarchy taunt
  arrived on the HUD, and the followers reached the player (server log).
- **Not right:** the F7 score screen draws the team headers and the selected pilot's panel but **no pilot rows**, on
  both levels flown; the Matcen and upstream clients drew the same server's rows, so the fault is on this client. The
  HUD's "Network Loss" and "Network Ping" labels show no values. Neither was looked into; neither was tried on Piccu's
  Windows release.
- **Sound:** not checked. The first windowed starts passed a one-dash `-nosound`, which the engine ignored, so
  OpenAL was live in those menus; nobody listened. The later runs used `--nosound`.
- **Not run at all:** mouse flight (see Running), the dedicated server (`--dedicated`), the tracker module, single
  player, movies, force feedback.

## Fixed fault (2026-10-23): wrong ammo counts, then a hang (was "levels with an addon weapon page")

Symptoms, first seen 2026-10-18 on Forge and blamed on its addon weapon page, then reproduced 2026-10-23 on a
Matcen server with a stock level: a picked-up Mass Driver gave 1 round, the next weapon read 32768, other slots
held values such as 32765 and 20819, and a few seconds later the game stopped responding and KWin's
"not responding" dialog killed it (SIGABRT from `kwin_killer_helper`, core dump in `coredumpctl`).

Cause: the Linux `min()`/`max()` templates in `lib/Macros.h` returned a dangling reference (see "The patch, file by
file"). Every same-type `min`/`max` in the engine read a dead stack slot. `AddWeaponAmmo` (`multisafe.cpp`) sizes
the pickup with `min(max_ammo - ammo, count)`, hence the ammo garbage. `WeaponDoFrame` (`WeaponFire.cpp`) clamps
the number of smoke-trail blobs per frame with `min(int_extras, 2)`; the unclamped value was about 1e9, so one
weapon spent the rest of the process in that loop. The core shows all 5000 vis effects as smoke blobs at one point,
created in the same frame, and the main thread inside `CreateFireball` from that loop. Piccu on Windows uses the
macros, which is why it never had this. The addon weapon page was a coincidence.

Fix: the templates return `std::common_type<A, B>::type` by value.
