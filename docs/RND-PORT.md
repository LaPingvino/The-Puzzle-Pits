# The Puzzle Pits in Rocks'n'Diamonds

`tools/pit2rnd.mjs` turns the levels into a Rocks'n'Diamonds level set. This is what each level needs, worked out from its scripts (`tools/pit.mjs` shows them) and from the rules built into PITS.C, and what Rocks'n'Diamonds can do about it.

## What R'n'D has, and doesn't

R'n'D keeps **one element a square**: Pits' two layers (the ground under, an object over) become one element for each pair that can happen, such as "a crate on a down arrow". Custom elements (CEs) do the rest. What they offer that Pits needs:

- **Moving into something:** a CE can enter given elements and leave another behind, which is how a box on a target, an arrow or a plate becomes its own element and gives the square back when it leaves.
- **Changing:** a CE changes when pushed, entered or left by the player, touched, or when another element changes anywhere on the level. A changing CE can turn its neighbours into other elements.
- **Moving by itself:** in a direction, or on in the direction it was pushed.
- **Actions:** end the level (`exit player`), show one of four texts (`show envelope`), count (`CE value`), give keys.

What has no place in R'n'D: typing an answer (`INPUT`), changing what the player can walk on (`WALKABLE`), moving the player to a square (`LET XGUY`), counting the player's moves, and the gems' powers.

## The rules built into Pits

| Pits | In R'n'D |
|---|---|
| Barrel, crate, chest; red crate, green chest, blue barrel on their own colour of target | a CE each, and one for each on its target |
| Arrows: an object on one moves a square each turn | an object-on-arrow CE for each direction, moving that way and leaving the arrow behind |
| Glass: an object pushed onto it slides on | an object-on-glass CE, moving on in the direction it was pushed |
| Water: a crate sinks to a bridge, a barrel floats (and can be pushed off) | the crate enters water and leaves a bridge; a barrel-in-water CE |
| Lava: a barrel or crate burns, a chest makes glass | enters lava and is gone; the chest leaves glass |
| Hole and wood: a barrel sticks, a crate fills a hole or breaks wood | as for water |
| Grass: no object goes onto it | an element the player walks over and boxes don't enter |
| Keys and locks | R'n'D's own keys and doors |
| Gems | collected, as R'n'D's gems; their powers (red lays arrows, green walks through walls, blue pulls) are not possible |
| Plates and switches, with a script | a CE that changes when pressed or flipped, and the elements that change with it |
| A wall with text | touched: `show envelope` (four a level) |

## Level by level

**Plain Sokoban** is how the set plays now: the squares a level is won on are its goals (from `Master`, when it wins on boxes standing on given squares), the rest is walls and floor. **Custom elements** is what it would take to play as in Pits. Most levels' scripts only set up their story (`Intro`) and decide the win (`Master`).

| Level | How it's won, and what's in it | Plain Sokoban | With custom elements |
|---|---|---|---|
| 1 Starting Out | a barrel on a plate frees a yellow gem that runs away; catch it | no | yes: a gem that moves away from the player, collected to win |
| 2 A Little Harder | walk through a wall at 8,6; at 8,7 a wooden bridge appears; reach 8,12 | no | yes: a wall the player walks into, a square that makes the bridge, an exit |
| 3 Pick Four | two barrels on 9,3 and 10,3 raise walls, then two on 9,4 and 10,4 | no | yes, in two stages |
| 4 Crossroads | four barrels on the middle four squares | **yes** | |
| 5 Hide And Go Seek | no barrel left in a room | no | yes: count the barrels in it |
| 6 Nothing Under The Boxes | a crate sunk at 14,13 drains the water; walk into the sign | no | yes |
| 7 Islands | crates on three squares; crates sink to bridges | goals right, water a wall | yes |
| 8 The Warehouse | blue barrels in three corners; the target moves | no | yes |
| 9 Just Do It | six barrels in a ring | **yes** | |
| 10 Time Limit | four barrels afloat in a column of water; arrows | no | yes |
| 11 The Factory, Part 1 | fill a room, then two barrels; arrows | no | yes |
| 12 The Factory, Part 2 | two columns of crates, then the player at 18,2; arrows, water | no | yes |
| 13 Shortcut | a plate and a switch open walls; a wall walkable from one square | no | partly |
| 14 Mixed Signals | no barrel left in a room; arrows | no | yes |
| 15 Labyrinth | a barrel to 2,2 | **yes** | |
| 16 3 Up, 3 Down | blue barrels on two squares open a way out; arrows | no | yes |
| 17 House of Glass | barrel, player, barrel in a row; glass; two squares swap the player | no | mostly (the player in the row isn't) |
| 18 Four Corners | the green gem's power, then 11,11 | no | no |
| 19 Take Your Best Shot | barrels in an L, either way round; glass | no | one of the two ways |
| 20 Symmetry | four barrels, one of them blue, on given squares | **yes** (any barrel will do) | |
| 21 Into The Deep | six plates raise a platform, step by step; it carries the player | no | partly |
| 22 Man At Work | a second worker the player steers | no | no |
| 23 The Magic Room | plates turn the arrows and make a room appear | goals right, plates do nothing | yes |
| 24 As Easy As Pie | an unseen path, taken in the right order | no | no |
| 25 Riddle Room | riddles, answered by typing | no | no |
| 26 Timing Counts | two crates on two plates at once; arrows | no | yes |
| 27 Watch Your Step | follow the arrows exactly | no | no |
| 28 Garbage In, Garbage Out | a plate turns barrels to crates; four crates in a row | goals right, the plate does nothing | yes |
| 29 The Secret Stream | reach 16,16, or a hidden way | no | yes |
| 30 The Museum | fill the pool; seven signs; keys | no | partly (four signs) |
| 31 Inferno | the red gem moves a lava square; keys | no | no |
| 32 Up Lava Creek | switches move lava; the blue gem makes glass | no | partly |
| 33 The Prime Directive | a number of moves | no | no |
| 34 The Garden | chests into lava on two squares (glass) | no | yes |
| 35 Gateways | marked squares open with gems | no | no |
| 36 Variations On A Theme | plates turn one object into another; glass on three squares | no | yes |
| 37 FireWalker | the floor burns behind the player; walk all 84 squares | no | yes: floor that leaves lava, a count to win |
| 38 Two Birds | a barrel on a sunk crate | no | yes |
| 39 Joystick Fun | a switch turns the arrows; a barrel to 2,4 | goals right, arrows floor | yes |
| 40 The End Of The Line | five red crates in a row; gems, lava, a plate | goals right, the rest isn't | partly |

So far: 4 levels play as in Pits, 5 more have the right goals. Custom elements would bring about 25 to Pits' rules, and about 8 (18, 22, 24, 25, 27, 31, 33, 35) rest on what R'n'D can't do.

## Doing it

Custom elements are stored in the level files (chunks `CUSX` and `CUS4`, in the same typed-value form as `INFO`; `src/files.c`). The way to go is one set of them for the shared rules, as a level template (`use_custom_template`), and per level only its own script's changes: first the boxes and targets in colour, water, lava and arrows (the rules every level uses), then the plates, switches and signs level by level.
