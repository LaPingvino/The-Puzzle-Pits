# The Puzzle Pits in Rocks'n'Diamonds

`tools/rnd.mjs` turns a level into a Rocks'n'Diamonds level that plays by Pits' own rules, made of R'n'D's custom elements and written into each level file. `tools/pit2rnd.mjs` makes Abe's forty into a level set; the level editor previews any level as R'n'D will draw it and downloads it, or a whole pack, for R'n'D. Nothing in R'n'D itself is changed: the set plays in R'n'D 4.4.2.4 as it is.

## How Pits is won

A level is won when **every coloured target holds its own colour**: a crate on a red target, a chest on a green one, a barrel on a blue one (`CheckLevelDone` in PITS.C). A script can win a level too, but mostly the scripts tell the story, give hints or offer a second way; five levels have no targets and are won by their scripts alone (22, 24, 25, 29, 33).

## How it's made

R'n'D keeps one element a square, so each pair Pits has on a square (a ground, and what stands on it) is an element of its own.

- **Grounds** (floor, targets, arrows, glass, water, lava, holes, grass, gems, keys...) are elements the player walks over; water, lava and holes they don't.
- **Boxes**: one element for each kind on each ground it can stand on ("a barrel on a blue target"), drawn on that ground. A box can enter any ground but grass; it leaves its ground behind and becomes the box on the ground it entered (R'n'D's "digging" event names it). So it is home on its own colour of target, carried by arrows (it moves by itself), sliding on glass (an element for each side it came in by, as a changed element forgets its direction), afloat in water, stuck in a hole, or gone: a crate in water sinks to a bridge, a barrel or crate burns in lava, a chest leaves glass there.
- **The win**: a counter, hidden in the void above the pit, starts at the targets still to fill; it counts down when a box comes home ("creation of" those elements), up when one leaves ("change of"), and at none the player exits.
- **The player** starts on a floor element that turns into them once (R'n'D keeps a walkable element under a player made from it).
- **Gems and keys** are collected by walking over them. The gems are used by clicking their buttons, in a row above the pit as in Pits' panel; a button is an element for each count up to 8, as a change can't look at a value.
  - **Red**: the floor and arrows are armed, and the one the player leaves becomes an arrow the way they went.
  - **Blue**: the floor is armed, and the one left becomes a magnet for a moment; a box on the floor behind it moves onto it.
  - **Green**: walls, water, lava and holes become ones the player can walk through, until the button is clicked again. A ghost still inside one then can step out.

How R'n'D's custom elements are stored (chunks `CUSX` and `GRPX`, typed values as in `src/files.c`) is in the code. Things that took finding out: a ground a box enters can't be indestructible; a group holds at most 16 elements (larger ones are groups of groups); a change on another element's doing needs R'n'D's "by other action" event as well; a change delay is `delay_fixed` times `delay_frames`; a level with mouse clicks waits for a key before it starts, so only levels with gems have buttons.

## What doesn't carry over

| | Levels |
|---|---|
| What plates, switches and signs do (their scripts) | 1, 6, 13, 21–28, 30, 32, 33, 36, 39, 40 |
| A win by scripts alone | 22, 24, 25, 29, 33 |
| Keys opening locks (a script runs there) | 30, 31 |
| The green gem through boxes, and its swap (a box where the ghost lands goes to where it started) | 5, 7, 11, 13, 18, 21, 23, 35, 40 |
| The blue gem pulls a box only from plain floor | |

18 levels play as in Pits (2–4, 8–10, 12, 14–17, 19, 20, 34, 37, 38, and 5, 7, 11, 18 and 35 unless their answers walk a ghost through boxes).

## Next

The scripts, level by level: most of what they do is a plate or switch changing squares, which a custom element can do (it changes, and others change with it). Four rest on what only a script can do: a second worker (22), a hidden path taken in order (24), typed riddles (25), a count of moves (33).

R'n'D itself could take two small additions for what's left, as change actions beside "move player": **pull** (the element on the far side of the player follows them) and **pass through** (the player goes through walls and objects for a while). Its author offered help with this port in 2020; such changes go to him as patches, as R'n'D isn't developed on GitHub.
