# The Puzzle Pits in Rocks'n'Diamonds

`tools/rnd.mjs` turns a level into a Rocks'n'Diamonds level: `tools/pit2rnd.mjs` makes Abe's forty into a level set, and the level editor shows any level converted, lets the conversion be corrected by hand, and downloads it (or a whole pack) for R'n'D. This is what the levels need, and what R'n'D can do about it.

## How Pits is won

A level is won when **every coloured target holds its own colour**: a crate on a red target, a chest on a green one, a barrel on a blue one (`CheckLevelDone` in PITS.C). That is each level's own answer, and what the conversion keeps: the targets are the goals.

A script can win the level too, and most of what the scripts do is a second way to the answer, a hint, or the story. Only five levels have no targets and are won by their scripts alone: 22, 24, 25, 29 and 33.

## The rules built into Pits

These are what each level's answer is made of, and what has to work in R'n'D. R'n'D keeps one element a square, so Pits' two layers (the ground, and what stands on it) become one element for each pair that can happen: a custom element (CE) for "a crate on a down arrow", and so on.

| Pits | Levels | In R'n'D |
|---|---|---|
| Barrel, crate and chest, each on its own colour of target | most | a CE each, one for each on its target; won when all are (the level's gems count, the exit opens at none) |
| Arrows: what stands on one moves a square each turn, not onto grass | 7, 10–12, 14, 16, 22, 23, 26, 27, 30, 33, 39 | an object-on-arrow CE for each direction, moving by itself and leaving the arrow behind |
| Glass: what is pushed onto it slides on | 17, 19, 33 | an object-on-glass CE, moving on in the direction it was pushed |
| Water: a crate sinks to a bridge, a barrel floats and can be pushed off | 6, 7, 10, 12, 13, 18, 19, 21, 22, 27, 29, 30, 31, 34, 35, 38, 40 | the crate enters water and leaves a bridge; a barrel-in-water CE |
| Lava: a barrel or crate burns up, a chest leaves glass | 31, 32, 40 | the box enters lava and is gone; the chest leaves glass |
| Holes and wood: a barrel sticks, a crate fills a hole or breaks wood | 25, 30, 32 | as water |
| A chest pushed against what it can't enter breaks, leaving glass | the chest levels | the chest changes when the player pushes it and it can't move |
| Grass: the player walks on it, nothing is pushed onto it | 34, 36, 39, 40 | a floor boxes don't enter |
| Keys open locks | 30, 31 | R'n'D's own keys and doors |
| **Gems' powers** (used by the player, one a gem): red lays an arrow on each square left, blue pulls what's behind, green walks through anything and swaps it back | 5, 7, 11, 13, 18, 21, 23, 30, 35, 38, 40 | the hard part, below |
| The yellow gem shows the whole pit | | not needed (R'n'D shows it) |

Everything but the gems' powers is within what custom elements do. Their powers have no counterpart in R'n'D: a CE can change the squares the player leaves, one way for each side, which could make the red gem's arrows, but there is no pulling and no walking through walls. Whether a level needs a gem depends on its answer: some are a help, some the way through. Those eleven levels are the ones to check by playing.

## Level by level

| Level | Its answer needs | Now | With custom elements |
|---|---|---|---|
| 1 Starting Out | barrels on blue targets | plays | |
| 2 A Little Harder | barrels on blue targets | plays | |
| 3 Pick Four | barrels on blue targets | plays | |
| 4 Crossroads | barrels on blue targets, crates in the way | plays (crates are barrels) | crates kept apart |
| 5 Hide And Go Seek | barrels on blue targets; green gems | plays, without gems | if the gems aren't needed |
| 6 Nothing Under The Boxes | water, crates | water is wall | yes |
| 7 Islands | water, crates, an arrow; green gems | water is wall | yes, if the gems aren't needed |
| 8 The Warehouse | crates in the way of barrels | plays (crates are barrels) | crates kept apart |
| 9 Just Do It | barrels on blue targets | plays | |
| 10 Time Limit | arrows, water | arrows are floor | yes |
| 11 The Factory, Part 1 | arrows; all four gems | arrows are floor | if the gems aren't needed |
| 12 The Factory, Part 2 | arrows, water, crates | | yes |
| 13 Shortcut | water, crates; green and blue gems | | if the gems aren't needed |
| 14 Mixed Signals | arrows | | yes |
| 15 Labyrinth | a barrel on a blue target | plays | |
| 16 3 Up, 3 Down | arrows | | yes |
| 17 House of Glass | glass | | yes |
| 18 Four Corners | water, crates; green and red gems | | if the gems aren't needed |
| 19 Take Your Best Shot | glass, water | | yes |
| 20 Symmetry | barrels on blue targets | plays | |
| 21 Into The Deep | water; a green gem; plates (its script) | | partly |
| 22 Man At Work | its script (a second worker) | no | no |
| 23 The Magic Room | arrows, crates; a green gem | | if the gem isn't needed |
| 24 As Easy As Pie | its script (a hidden path) | no | no |
| 25 Riddle Room | its script (riddles typed in) | no | no |
| 26 Timing Counts | arrows, crates on red targets | | yes |
| 27 Watch Your Step | arrows, water, a crate | | yes |
| 28 Garbage In, Garbage Out | barrels on blue targets | plays | |
| 29 The Secret Stream | its script (reach a square), water, chests | no | yes: an exit there |
| 30 The Museum | arrows, water, keys, all the boxes; red and blue gems | | partly |
| 31 Inferno | lava, crates on red targets, a key | | yes |
| 32 Up Lava Creek | lava, wood, chests | | yes |
| 33 The Prime Directive | its script (a number of moves) | no | no |
| 34 The Garden | grass, water, chests and a barrel | | yes |
| 35 Gateways | water, all the boxes; red and blue gems | | if the gems aren't needed |
| 36 Variations On A Theme | grass, a crate on each colour of target (its plates change it) | | yes, with the plates |
| 37 FireWalker | barrels on blue targets | plays | |
| 38 Two Birds | water, a crate and a barrel; a red gem | | if the gem isn't needed |
| 39 Joystick Fun | grass, arrows | | yes |
| 40 The End Of The Line | grass, lava, water, every box; red gem | | if the gem isn't needed |

So: 11 levels play now; custom elements for the built-in rules would bring about 20 more, and the eleven with gems hang on whether their answers use them. Four (22, 24, 25, 33) rest on what only their scripts do.

## Doing it

Custom elements are stored in the level files (chunks `CUSX` and `CUS4`, in the same typed-value form as `INFO`; `src/files.c`). The rules are the same in every level, so they go in one set of custom elements, a level template (`use_custom_template`), and the conversion writes each Pits square as the CE for its ground and object. A level's scripts can come after, by hand in the editor's corrections where they matter.
