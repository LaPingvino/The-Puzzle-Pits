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

## Make your own levels

The level editor is online at **https://lapingvino.github.io/The-Puzzle-Pits/editor/**, and each release has it as one HTML file to use offline (`puzzle-pits-editor.html`). It opens Abe's levels or any `.PIT` file, paints the ground and the objects, and edits the levels' scripts as text. It saves `.PIT` files that both this game and the original DOS `PITS.EXE` read: put one in the `LEVELS` folder as `LEVELnn.PIT`. **Play** tries the level in the web game, and on the desktop `puzzle-pits mylevel.pit` plays a level file straight away.

### Level packs

As in Rocks'n'Diamonds, levels come in packs. Abe's forty are the first; any other is a folder of `LEVEL01.PIT`, `LEVEL02.PIT` and so on, with an optional `pack.txt` holding `name=` and `author=` lines. The game looks for them in a `PACKS` folder beside its data, and in `packs` in its user data folder (`~/.local/share/PlasmaWorks/PuzzlePits/packs` on Linux). When there are more packs than Abe's, the title screen's first line picks one with Left and Right, and each pack remembers its own progress. The editor makes them: add levels to its pack, then **Download pack (.zip)**, and unpack the zip into `PACKS`; it opens packs again from their zip. Packs aren't in the web version of the game yet.

The level format and the script language are described in [docs/PIT-FORMAT.md](docs/PIT-FORMAT.md); `node tools/pit_test.mjs` checks that every one of Abe's levels reads and writes back byte for byte. `node tools/build-editor.mjs editor.html` builds the editor.

## Rocks'n'Diamonds

Pits is a Sokoban variant, so its levels also play in [Rocks'n'Diamonds](https://www.artsoft.org/rocksndiamonds/) as a Sokoban level set (R'n'D's own level files, and the same levels as `levels.xsb` for other Sokoban games), drawn with the game's own pictures: `node tools/pit2rnd.mjs PITS95/LEVELS the_puzzle_pits`, then put the folder in `~/.rocksndiamonds/levels` (each release has it zipped). Barrels, crates and chests become Sokoban's boxes and the coloured targets its goals. Water, lava and holes become walls, and keys, arrows, gems and the stories are left out. The set's README.txt says what each level loses; levels 2, 3, 9, 15, 20 and 37 are exactly as in Pits.

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
