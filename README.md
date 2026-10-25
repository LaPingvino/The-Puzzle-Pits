# The Puzzle Pits

A puzzle game from 1995 by Abe Pralle (Plasma Works), first for DOS, now for Linux, Windows, macOS and the web. Push barrels, step on plates, mind the lava, and find the way out of forty pits.

The source was released under the Apache 2.0 License with Abe's permission (see README), and the game's data may be freely distributed (see PITS95/readme.txt).

## Play

Play it online at **https://lapingvino.github.io/The-Puzzle-Pits/**, or download it:

the [releases](https://github.com/LaPingvino/The-Puzzle-Pits/releases) have it for each system: unpack it and start `puzzle-pits`.

- **Linux:** SDL2 needs to be installed. On most systems it is already; if not, install `sdl2` or `libsdl2-2.0-0`.
- **Windows:** SDL2 comes with the game.
- **macOS:** SDL2 comes with the game. The first time, open it with right-click > Open, because it isn't signed.

The game remembers the furthest level you reached: **Resume** on the title screen starts there, and **Choose Level** (Left and Right) any level before it. The arrow keys or the mouse move you; Enter or a click goes on. The F keys use what you carry, as shown beside them. F11 or Alt+Enter switches to full screen. The game's own instructions are in PITS95/readme.txt.

## Build

You need [Zig](https://ziglang.org) 0.17 and SDL2's development files:

```sh
zig build            # zig-out/bin/puzzle-pits, and the demo
zig build run        # play
```

Cross-compiling for Windows uses SDL2's own Windows build: unpack `SDL2-devel-…-mingw.tar.gz` from [SDL's releases](https://github.com/libsdl-org/SDL/releases) and run:

```sh
zig build -Dtarget=x86_64-windows-gnu -Dsdl-prefix=SDL2-…/x86_64-w64-mingw32
```

`zig build web` builds it for a browser into zig-out/web, with [Emscripten](https://emscripten.org) (its `emcc` on the PATH).

`zig build -Dsdl=false` builds without SDL2; it compiles, but shows and plays nothing.

## How the port works

The game's own code is kept as it was. The parts of DOS it relied on are replaced:

- **Screen:** VGA mode 13h, 320×200 in 256 colours, is shown through SDL, which scales it to the window. Video memory is shown continuously, as the VGA card did, so palette fades work as they did.
- **Sound:** DiamondWare's mixer for the Sound Blaster runs in an SDL audio callback (`sdl_compat.c`).
- **Keyboard and mouse:** these come from SDL events.
- **Compiler:** the game was written for a 16-bit DOS compiler. It's built with unsigned `char`, and `WORD`, `LONG` and `DWORD` have the sizes they had there (16, 32 and 32 bits).

To test without a window, run the game with `SDL_VIDEODRIVER=offscreen` from PITS95, with these settings:

- `PITS_KEYS="13000:Return,15000:Right,16000:@60;40,16500:Click"` presses keys at those times (milliseconds), moves the mouse and clicks.
- `PITS_QUIT_AFTER=20000` makes the game quit by itself after that many milliseconds.
- `PITS_SHOT=screen.ppm` writes what's on screen when the game ends.
