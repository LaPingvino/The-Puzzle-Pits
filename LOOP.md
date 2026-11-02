# The Puzzle Pits — loop

One bounded chunk (~10 min) per tick, top unchecked item first. Commit and push to origin master each tick
(the web build deploys to https://lapingvino.github.io/The-Puzzle-Pits/).

## Non-negotiables

- Abe's game plays as it did: docs/PITS-RULES.md is the reference for behaviour; the web page around it may
  change, the game's rules may not.
- Keep the desktop build working (zig build; CI).

## Queue

1. [x] Mobile — done but for a real-phone try (iOS) and the menu question below (Joop 2026-11-01: "Pits mobile support! … for all other gh pages projects as well"; approved):
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

## Questions for Joop

- (ANSWERED 2026-11-02, Joop: 'Making lines tappable in Pits is ok!') The title menu takes only keys (arrows + Enter, or e/s/q): on a phone it works through the Keyboard button (type s). Make the menu lines tappable (a tap on a line chooses it)? That touches Abe's menu code (interface, not rules), so asking first.

## Blocked

- (DONE 2026-11-01) Abe invited as collaborator: brombres (write; pending his acceptance).
- (RESOLVED 2026-11-01) emscripten installed. Local web build: PATH=/usr/lib/emscripten:$PATH
  EM_CACHE=$TMPDIR/emcache ZIG_GLOBAL_CACHE_DIR=… zig build web (the first run fails once while emscripten
  generates its libraries; the second succeeds). Headless check: chromium --timeout=20000 (not
  --virtual-time-budget: the game loop never lets virtual time settle).

## Tick log
- 2026-11-01 tick 1 (item 1a) — web/shell.html: viewport without user scaling, touch-action/overscroll none, no text selection; fit() keeps whole-number scaling but uses the largest fractional scale when that would be under 2× (a phone). Local build + headless: 844×390 → 1.52× (was 1×), 1280×800 → 3× (unchanged). NEXT: 1b on-screen controls (touch screens only).
- 2026-11-01 tick 2 (item 1b) — read the input code: taps (as mouse) already cover moving, Enter/Space waits and the panel's items; plan revised (no d-pad/gem buttons). NEXT: 1c — the phone keyboard for typed text (riddles, password): a 'keyboard' button on touch screens focusing a hidden input whose characters reach SDL as key events; verify in headless Chromium.
- 2026-11-01 tick 3 (item 1c, NOT YET VERIFIED IN GAME) — a Keyboard button (touch screens, or ?kbd) focuses an off-screen input; each typed character goes to the game through pits_type() (sdl_compat.c: a 64-char queue read after the key from SDL, as inkey), Backspace as 8 (two spaces kept in the box), Enter as 13; the box's key events don't reach SDL (no doubles). Built (zig build web), export present, button shows; headless Chromium (swiftshader, --timeout up to 42 s, PITS_KEYS via ENV in preRun) stays on the opening logo, so the password screen check is open. NEXT: get the headless run past the logo (or a native test calling pits_type), then type 'abc' + Backspace on the password screen; then 1d.
- 2026-11-01 tick 4 (item 1c, verified) — PITS_KEYS learns 'text (typed through pits_type, \b = Backspace); native run with SDL_VIDEODRIVER=offscreen: Return, Return, 'abc\bd → the password screen shows 'abd_'. 1c done. NEXT: 1d sound on first tap (check the web build's audio unlock on touchstart).
- 2026-11-01 tick 5 (item 1d) — sound from the first tap: web/shell.html resumes SDL2's AudioContext (Module.SDL2.audioContext) inside each touchend/pointerdown/keydown until it runs (Emscripten's SDL2 resumes it only from its audio callback, which iOS Safari refuses). Checked with Chromium's DevTools protocol, autoplay requiring a gesture: 'suspended' before, 'running' after one trusted tap. Item 1 now waits only on a real-phone try (iOS especially).
- 2026-11-01 tick 6 (item 1, check) — touch emulation over the DevTools protocol (real taps on the 844×390 page): the menu ignores taps (keys only), 's' typed via the Keyboard box starts a game, taps turn the intro's pages, a finger held left/right of the player walks him (the view scrolls, the pointer shows the arrow). Item 1 done but for a real phone; asked Joop about a tappable menu.
- 2026-11-02 (Joop's answer, recorded from the goloco session) — tappable title-menu lines approved (interface only; Abe's rules untouched): workable next Pits tick.
- 2026-11-02 (goloco session's Pits turn; tappable menu, approved by Joop) — written in PITS.C's title-menu loop (the menu only: a tap on a line chooses it; on 'Choose Level < nn >' and the pack line a tap on an arrow turns it; it waits for the finger to lift, as Abe's code does elsewhere; rows start 2 px above `top`, where ShadowText draws) — but NOT VERIFIED, so it is on a local branch, wip/tappable-menu (not pushed), and master is unchanged. Headless check (zig build web, Chromium + the DevTools protocol, touch emulation as in tick 6, and a mouse click): a debug print in the menu loop showed mx / my / button1 never change while the menu waits — no pointer or button event reaches the game there at all (the pointer stays at its start, 108,84), though the menu is on screen. Next: find why (does the menu's loop yield to the browser so SDL's events arrive — CheckMouse → readmbutton → process_sdl_events; do taps reach the intro pages in this same run; Emscripten's SDL touch-to-mouse needs the canvas to get the events), then verify, merge and push.
