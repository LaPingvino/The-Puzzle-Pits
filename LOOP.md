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
   (b) on-screen controls on touch screens only — REVISED after reading PITS.C: almost every Enter/Space wait
   also takes a click (button1: lines 602, 947–951, 1287–1289, 2045–2065, 2358–2360), the board moves the
   player toward the pointer, and the panel's gems, keys, yield and stop are clickable boxes (2085–2263),
   with F1–F10 only as keyboard shortcuts. SDL turns touches into the mouse, so taps already do all of it;
   no d-pad or gem buttons (they would duplicate the panel). Left: confirm on a touch emulation; Escape if
   the game needs it on a phone;
   (c) the phone keyboard when the game wants typed text (the riddles, level 25);
   (d) sound starts on the first tap.
   Check: tapping already moves the player (SDL maps touches to the mouse) — confirm in a mobile emulation
   (headless Chromium with touch) before building on it.

## Blocked

- (DONE 2026-11-01) Abe invited as collaborator: brombres (write; pending his acceptance).
- (RESOLVED 2026-11-01) emscripten installed. Local web build: PATH=/usr/lib/emscripten:$PATH
  EM_CACHE=$TMPDIR/emcache ZIG_GLOBAL_CACHE_DIR=… zig build web (the first run fails once while emscripten
  generates its libraries; the second succeeds). Headless check: chromium --timeout=20000 (not
  --virtual-time-budget: the game loop never lets virtual time settle).

## Tick log
- 2026-11-01 tick 1 (item 1a) — web/shell.html: viewport without user scaling, touch-action/overscroll none, no text selection; fit() keeps whole-number scaling but uses the largest fractional scale when that would be under 2× (a phone). Local build + headless: 844×390 → 1.52× (was 1×), 1280×800 → 3× (unchanged). NEXT: 1b on-screen controls (touch screens only).
- 2026-11-01 tick 2 (item 1b) — read the input code: taps (as mouse) already cover moving, Enter/Space waits and the panel's items; plan revised (no d-pad/gem buttons). NEXT: 1c — the phone keyboard for typed text (riddles, password): a 'keyboard' button on touch screens focusing a hidden input whose characters reach SDL as key events; verify in headless Chromium.
