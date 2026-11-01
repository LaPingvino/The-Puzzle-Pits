# The Puzzle Pits — loop

One bounded chunk (~10 min) per tick, top unchecked item first. Commit and push to origin master each tick
(the web build deploys to https://lapingvino.github.io/The-Puzzle-Pits/).

## Non-negotiables

- Abe's game plays as it did: docs/PITS-RULES.md is the reference for behaviour; the web page around it may
  change, the game's rules may not.
- Keep the desktop build working (zig build; CI).

## Queue

1. [ ] Mobile (Joop 2026-11-01: "Pits mobile support! … for all other gh pages projects as well"; approved):
   (a) the page fills a phone screen: landscape, a fractional scale when the whole-number scale would leave
   the 320×200 screen tiny, no page scroll/zoom under the fingers (touch-action, viewport);
   (b) on-screen controls on touch screens only: a direction pad, Enter, and the gem buttons for F1–F10 as the
   game's panel shows them;
   (c) the phone keyboard when the game wants typed text (the riddles, level 25);
   (d) sound starts on the first tap.
   Check: tapping already moves the player (SDL maps touches to the mouse) — confirm in a mobile emulation
   (headless Chromium with touch) before building on it.

## Blocked

- (DONE 2026-11-01) Abe invited as collaborator: brombres (write; pending his acceptance).
- 2026-11-01: no local web build — zig cc can't stand in for emcc (no wasm32-emscripten libc, no SDL port or
  JS glue). Waiting for Joop to install emscripten (`sudo pacman -S emscripten`; emcc in /usr/lib/emscripten).
  web/shell.html already has item 1(a) (fractional fit under 2×, no pan/zoom) — uncommitted until it is
  checked in a local build.

## Tick log
