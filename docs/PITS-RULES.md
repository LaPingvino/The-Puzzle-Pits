# The Puzzle Pits: how the game plays

This describes how The Puzzle Pits behaves, completely enough to make the game again: what is on a square, what the player and the objects do, what happens each turn, how scripts run, and how the pit is drawn. It goes with [PIT-FORMAT.md](PIT-FORMAT.md), which describes the level files and the script language's encoding. It describes behaviour, not the original's code.

Where the original does something odd that Abe's levels rely on, it is described as it is. Where it does something odd that no level relies on, the description says so and gives the sensible behaviour.

## 1. Files

A level set is a folder with:

- `LEVEL01.PIT`, `LEVEL02.PIT` ...: the levels (PIT-FORMAT.md).
- `TILES1.DAT`, `TILES2.DAT`, `TILES3.DAT`: the pictures, called *shapes*. Each file is a run of shapes, one after another: a byte with the width, a byte with the height, then width × height bytes, one palette index a pixel, row by row. A shape with width 0 ends the file. Shapes are numbered from 0 across the three files in order: the first shape of TILES1.DAT is 0, and TILES2.DAT goes on where TILES1.DAT ended. The game's squares are 24 × 24 pixels.
- `CRACK.WAV`, `GLASS.WAV`, `LAVA.WAV`, `MAGIC.WAV`, `PLATEOFF.WAV`, `PLATEON.WAV`, `SCRAPE.WAV`, `SPLINTER.WAV`, `WATER.WAV`: the sounds (section 9).

### The palette

Palette index *i* has the colour below, in 6 bits a component (0 to 63, as a VGA has them; for 8 bits, `c × 4 + c ÷ 16`). Indexes past the list are black. Index 0 in a shape drawn *through its mask* is see-through.

```
 0: 0 0 0     1: 63 63 63   2: 54 54 54   3: 50 50 50   4: 42 42 42   5: 37 37 37
 6: 25 25 25  7: 63 0 0     8: 54 0 0     9: 46 0 0    10: 37 0 0    11: 63 63 0
12: 63 42 0  13: 0 63 63   14: 29 46 63  15: 33 0 33   16: 33 63 33  17: 0 63 0
18: 0 46 0   19: 0 33 0    20: 0 33 63   21: 0 25 58   22: 0 16 50   23: 0 8 46
24: 0 4 37   25: 0 0 33    26: 63 37 33  27: 46 21 21  28: 33 16 0   29: 29 12 0
30: 25 8 0   31: 21 4 0    32: 0 0 0     33: 31 31 31  34: 27 27 27  35: 25 25 25
36: 21 21 21 37: 18 18 18  38: 12 12 12  39: 31 0 0   40: 27 0 0    41: 23 0 0
42: 18 0 0   43: 31 31 0   44: 31 21 0   45: 0 31 31  46: 14 23 31  47: 16 0 16
48: 16 31 16 49: 0 31 0    50: 0 23 0    51: 0 16 0   52: 0 16 31   53: 0 12 29
54: 0 8 25   55: 0 4 23    56: 0 2 18    57: 0 0 16   58: 31 18 16  59: 23 10 10
60: 16 8 0   61: 14 6 0    62: 12 4 0    63: 10 2 0
```

## 2. The pit

A pit is 23 × 23 squares, x and y from 0 to 22; the squares along the edge (0 and 22) are always empty, so the playable squares are 1 to 21. Each square has two layers: the *ground* (`under`) and what *stands* on it (`over`). The tile numbers are in PIT-FORMAT.md; the names used here:

| Over | | Ground | |
|---|---|---|---|
| 0 | nothing | 0 | the void (nothing at all) |
| 1 | wall | 53 | floor |
| 5 | wall with text (a sign) | 54, 55, 56 | red, green, blue target |
| 9 | pillar | 57 | marked tile |
| 13, 17 | switch, off and on | 58, 59 | plate, up and down |
| 21 | barrel in water | 60–63 | arrow up, right, down, left |
| 25 | barrel in a hole | 64–67 | gem: red, green, blue, yellow |
| 29 | barrel | 68–70 | key: red, green, blue |
| 33 | blue barrel (on a blue target) | 71 | glass |
| 37 | crate | 76–78 | lock: red, green, blue |
| 41 | red crate (on a red target) | 79 | crate in water (a bridge) |
| 45 | chest | 80 | crate in a hole |
| 49 | green chest (on a green target) | 81 | wood |
| 100 | the player (in the file only) | 82 | grass |
| | | 83 | hole |
| | | 84 | water |
| | | 88 | lava |
| | | 92, 96 | lava cooling (first, second step) |

Objects are drawn over two by two squares (section 8), so each object number is the first of four shapes; the stored value is always that first one.

The player is stored in the file as over 100 on the square they start on. When the level starts, that square's over becomes 0 and the player's position is kept apart, they face north, and that square is their last square (section 3). A level without a player starts them on 1, 1.

Two over tiles are boxes *at home*: a blue barrel is a barrel on a blue target, a red crate a crate on a red target, a green chest a chest on a green target. A box's colour follows its ground (the tile rules, below): it is never placed at home except on its own target.

## 3. What the game keeps

- The pit's two layers, and for each square which script runs there (the *script map*; 255: none).
- The player's position, and the square they were on before the last move.
- The direction the player faces: north 0, east 4, south 8, west 12.
- **Walkable**: for each tile number 0 to 100, whether the player can walk on it. At the start of a level, the ground tiles 53 to 82 are walkable and nothing else is (so not holes, water or lava; nor any over tile). The player can enter a square when its over is 0 or walkable, and its ground is walkable. Scripts change this table.
- **Last pushed**, for each square: the way the object there was last moved (north 0, east 1, south 2, west 3), or none (−1). It is set when an object is pushed or carried (to the way it went) and cleared on the square it left; when a script changes a square, that square's is cleared.
- Counters: the moves made; the gems and keys carried (red, green, blue gems; the yellow gem: carried or not; red, green, blue keys); for each colour, the targets of that colour and the boxes at home on them.
- Which gems are in use: red, green (with the stages of the green gem's change, section 6) and blue, each on its own (they can be in use at the same time).
- Ten script registers, R0 to R9, set to 0 when a level starts.
- The text the player last typed (for `INPUT`), and the number it starts with.

## 4. A turn

The game runs in *turns*. A turn is when the player does something: steps, pushes, waits, or uses a gem or key. Nothing moves between turns: arrows and glass move objects, plates are pressed, scripts run, all as part of a turn. Pits takes at most one turn every 9 ticks of its clock, which runs at about 60 ticks a second; held keys repeat at that pace.

### The player's input

- **A direction** (a key held): the player faces that way and tries to step that way (below).
- **Wait** (Return or Space with no direction): the player stays where they are, their own square becomes their last square (so `LASTX`, `LASTY` are their position: some levels' scripts take this as "the player waited"), and the turn's checks follow as after a step; the moves count goes up by one after the checks (after a step, before them), so scripts see the count before the wait.
- **The mouse**: pointing at a square turns the player towards it; a mouse button held steps that way, one square a turn, as a direction key. Turning by the mouse to the opposite direction goes through a quarter turn first (east to south to west, and so on), a turn each. The other button walks to the square pointed at, along the shortest way (below).
- **F1 to F4**: use the red, green, blue or yellow gem (section 6). **F5 to F7**: use a red, green or blue key (section 6).
- **Restart and quit** start the level again or end it.

### A step

When the player tries to step to the next square in a direction:

1. If that square is outside 1 to 21, nothing happens.
2. If the player can enter it (section 3), they step there, unless they were pushing (below): then this turn only ends the pushing, and the step comes next turn. A step: the square left is remembered as the last square; the red and blue gems act on it (section 6); the player moves; the walking picture goes on (section 8); the moves count goes up by one; then the turn's checks (below).
3. If the square's ground can't be entered (the over could), nothing happens, and pushing and walking to a square stop.
4. If something stands there that can't be walked on:
   - While walking to a square: the walk stops, and if the destination is further than the next square, the way there is found again (as it was blocked).
   - A barrel, crate or chest (29 to 52, at home or not): the player pushes it (below).
   - A barrel in water or in a hole (21 to 28): nothing; it can't be pushed.
   - A wall with text: its square's script runs, switched on (section 7).
   - A switch: it is flipped, off to on or on to off, and its square's script runs, switched on or off as it now is.
   - Anything else: nothing.

### Pushing

Pushing takes a moment: the first time the player tries to step into a box, nothing moves, but the pushing starts, and from 25 ticks later each turn the player still pushes moves the box. After a box has moved, the next one comes 25 ticks later again. Stepping away (or into a square they can enter) ends the pushing.

The box moves to the square beyond it, if:

- that square's over is 0 or walkable, and its ground isn't grass; or
- a chest stands there (a chest or a green chest): that chest breaks. Its square's ground becomes glass, and the box takes its place.

Then the box moves (as *moving an object*, below), the moves count goes up by one, and the player steps into the box's square if its ground is walkable for them (the square left behind, the gems and the walking picture as in a step). Then the turn's checks.

If the box can't move: a chest (or green chest) breaks where it stands, its ground becomes glass, the moves count goes up by one, and the player steps there as above (then the turn's checks); any other box stays.

### Moving an object

When an object moves from one square to the next (pushed, carried by an arrow, slid on glass): it leaves its square (over 0), arrives on the next, the tile rules (below) are applied to both squares, the square it came to gets the way it went as *last pushed*, and the one it left none. A square an object has arrived on this turn, however it came (pushed, carried by an arrow, slid on glass), takes no further part in this turn's arrows and glass: a box pushed onto an arrow moves on from the next turn, not in the same one. Only these three take a square out of this turn's arrows and glass. An object put on a square any other way (the blue gem's pull, the green gem's return, `SWAPOVER`, `CHANGEMAP`, a script setting `OVER`) keeps that square's place: if the square had an arrow or glass at the end of the last turn, that arrow or glass moves the object on in this same turn.

### The tile rules

Whenever a square's over or ground changes (moving, scripts), the rules for what stands there and its ground are applied:

First, what stands there takes its plain form again: a blue barrel, red crate or green chest is a barrel, crate or chest again (and its colour's count at home goes down by one); a barrel in water or in a hole is a barrel. Then, by what stands there and its ground:

| Ground | Barrel | Crate | Chest |
|---|---|---|---|
| the void (0) | falls: gone | falls: gone | falls: gone |
| water | floats: a barrel in water; *water* sound | sinks: gone, the ground a crate in water (a bridge, walkable); *water* sound | stays (a chest on water) |
| lava, lava cooling | burns: gone, the ground lava cooling (first step); *lava* sound | burns: as the barrel | the ground glass, the chest gone; *crack* sound |
| hole | sticks: a barrel in a hole; *splinter* sound | gone, the ground a hole; *splinter* sound | gone (the hole stays) |
| wood | breaks through: a barrel in a hole (the wood stays); *splinter* sound | gone, the ground a hole; *splinter* sound | stays |
| its own colour of target | at home: blue barrel; blue's count at home up by one | at home: red crate; red's count up by one | at home: green chest; green's count up by one |
| anything else | stays | stays | stays |

### The turn's checks

After the player has stepped, pushed or waited, in this order:

1. **The moves**: the scripts' `MOVES` is the moves made modulo 100, `HECTOMOVES` the hundreds.
2. **What the player stands on**: a gem or key is picked up (the ground becomes floor): a red, green or blue gem or key adds one to its count; the yellow gem makes it carried. (Not while the player is a ghost, section 6.)
3. **Scripts, switched on**: the `Master` script; then the `RedGem` script if the red gem is in use, the `GreenGem` script if the green one is, the `BlueGem` script if the blue one is (in that order).
4. **Arrows and glass**: every square with an arrow or glass on it, where an object stands (an over of 29 or more). The squares are those found at the end of the last turn's checks, in reading order (top row first, left to right), so all arrows and glass take part, however they came there (glass from a chest breaking too); squares a script turns to arrows during this turn come after them. An arrow the red gem lays during this turn's step is not among them: it takes part from the next turn (so an object the blue gem pulls onto it in the same step stays there this turn). (The blue gem's pull is not *moving an object*: section 4, "Moving an object".)
   - On an arrow: the object moves one square the way the arrow points, if that square isn't the player (unless they are a ghost), its over is 0 or walkable, and its ground isn't grass (the *magic* sound).
   - On glass: if the object has a *last pushed* direction, it moves one square that way, by the same conditions (the *glass* sound); if it can't, its last pushed is cleared (it stops).
   - (Only squares in columns 1 to 20 take part: in the original, the list of these squares can't hold column 21, so arrows and glass there don't move anything.)
5. **Every square**, top row first: the targets of each colour are counted again; a box at home on a square that isn't its target any more takes its plain form (and its count goes down). Then by what stands there (the player counts as standing on their square):
   - Nothing, on a plate that is down: the plate is *released* (noted, below).
   - The player, on a plate that is up: the plate goes down (*plate on* sound) and is *pressed* (noted). A ghost (section 6) doesn't press plates.
   - A barrel, crate or chest: the tile rules for it are applied again (as in the table), and on a plate that is up, the plate goes down and is pressed (noted).
6. **The plates noted**, the last one noted first: a pressed plate's script runs switched on (*plate on* sound); a released plate's script runs switched off. A released plate stays down: its script is what puts it up again (`POPUP`).
7. **The `Master` script, switched off.**

### Holding a key

Pushing ends when no direction is held: each new push waits the 25 ticks again. A push attempt that moves nothing ends the turn without the checks. A direction held into a switch flips it again every turn (each turn tries the step anew); walking into a switch or a wall with text is a turn for the 9-tick pace too, so a second press that comes and goes within those 9 ticks does nothing (the switch flips once), and a sign's script runs every turn the player keeps walking into it; held mouse button steps don't repeat on a switch (the button has to be let go first). Using a gem or key happens at once, without the turn's checks and without counting a move.

### From level to level

Gems and keys carried when a level is won are carried into the next level; restarting a level gives back what was carried when it started. A level started on its own (the original's password entry, choosing a level) starts with none. Levels count on this: level 12 needs a red gem saved from level 11 ("If you don't have one you'll have to redo Level 11"). Carried means the counts (red, green, blue gems, the yellow gem, keys); no gem is in use when a level starts: a red, green or blue gem still in use when the level was won is used up (one less of it carried in), and is out of use.

### Winning

The level is won when the boxes at home on targets are as many as the targets, and there is at least one target (the counts of all colours together); or when a script says `WIN`. This is checked after each turn.

## 5. Walking to a square

Clicking a square (the walking button) finds the shortest way there over squares the player can enter (section 3; a ghost can go anywhere), spreading out from the destination one square at a time, checking south, west, north and east in that order. If the player's square is reached, they walk the way found, one step a turn; a step that would turn them right round first turns them a quarter (from north, a step south first turns them west; from east, west first turns them north; from south, north first east; from west, east first south), a turn each. The walk ends when they arrive, when a direction is pressed, or when they can't go on (it is found again if the way was blocked by something standing in it).

## 6. Gems and keys

- **The red gem** (F1): if one is carried and it isn't in use already, it is in use (*magic* sound; the `RedGem` script runs switched on). Pressed again, it is put away: the `RedGem` script runs switched off, and one red gem is used up. While it is in use, and the level has no `RedGem` script, each square the player steps off becomes an arrow the way they went, if it was floor or an arrow.
- **The blue gem** (F3): put in use the same way (the `BlueGem` script runs switched on). While it is in use, and the level has no `BlueGem` script, the next step pulls what stands behind the player (on the square the other way from where they step), if it is an object (21 or more): it moves into the square the player left (taking that square's *last pushed* as east, west, south or north; these aren't directions glass knows, so a pulled box doesn't slide), and the tile rules are applied to both squares. Then the blue gem is used up (one less) and out of use. Pressed again before a step: put away, used up, its script run switched off.
- **The green gem** (F2): if one is carried and it isn't in use already, it is put in use: the *magic* sound, and the `GreenGem` script runs switched on. **With a `GreenGem` script**, that is all: the player does not become a ghost, can enter only the squares they could anyway, and picks up what they stand on as usual; the script runs each turn while the gem is in use (section 4). Pressed again, if the ground they stand on is walkable: the gem is out of use and used up, and its script runs switched off; nothing else happens (nothing is moved, nothing is remembered as a start square). **Without a `GreenGem` script**, the player becomes a ghost: they turn to face south (from north through east), and change into a ghost in five steps (section 8). The square they started on is remembered. As a ghost, the player can enter any square, and nothing they stand on is picked up; scripts reading what stands on their square see what is really there, not the player. Pressed again, the ghost becomes the player again, if they are on a square whose ground is walkable, and either nothing stands there, or the square they started on is empty and what stands here is an object (21 or more) or walkable. Then something standing there that can't be walked on moves to the square they started on (with the tile rules on both); the player faces south, changes back in six steps, and what they stand on and the turn's checks follow as after a step. The gem is used up, out of use, and its script (if any) runs switched off. On a square where they can't become the player again, nothing happens. The change (either way) takes a moment to show; a gem or key pressed meanwhile isn't lost, but acts as soon as the change is done.
- **The yellow gem** (F4): once carried (it is not used up), it shows the whole pit, and nothing else happens.
- **The keys** (F5 red, F6 green, F7 blue): with one carried, standing on a lock of that colour: one key is used, the lock becomes a plate that is down, and that square's script runs switched on.

`REDGEMOFF`, `GREENGEMOFF` and `BLUEGEMOFF` in a script put a gem in use away as pressing it again does.

The three gems are independent: each one is put in use and away by itself, whether or not another gem is in use (a red gem laying arrows, the green gem's ghost and a blue gem's pull can all be in use together). When the player steps off a square with both the red and the blue gem in use, the red gem's arrow is laid first, then the blue gem's pull moves the object onto that square.

## 7. Scripts

The language and its encoding are in PIT-FORMAT.md; this is what running a script does.

A script runs *switched on* or *switched off*, and *at* a square (the square it was run for, which sets `XPOS` and `YPOS`; for `Intro`, `Master` and the gem scripts, 0, 0). It runs from its first command:

- `SWITCHON` … `SWITCHOFF` …: switched on, what follows `SWITCHON` runs, and reaching `SWITCHOFF` ends the script; switched off, the script goes on from just after `SWITCHOFF`. A script without them runs the same either way.
- `IF` … `ELSE` … `ENDIF`: if the condition holds, what follows runs until `ELSE` (which then goes on after `ENDIF`) or `ENDIF`; if not, what follows `ELSE` runs, or nothing. Conditions compare two values: `EQ`, `NE`, `LT`, `GT`, `LE`, `GE`; `IF INPUT "text"` holds if the text the player last typed starts with that text, not minding capitals.
- `FOR r a b` … `NEXT`: r is set to a, and what is between runs; at `NEXT`, if r hasn't reached b, it goes one towards b (up if b is more than a, else down) and it runs again. It always runs at least once.
- `STOP` and `END` end the script. `WIN` ends it and wins the level.
- `REDRAW` draws the pit as it is now; `APPEAR` draws it with the *magic* sound (the original fades it in).
- `PRINT "text"` shows the text and waits for the player to go on; `\n` (character 10) starts a new line. Text that doesn't fit is shown a page at a time. `INPUT "text"` shows the text and asks the player to type a line (up to 16 characters): that is what `IF INPUT` compares, and the number it starts with (0 if none) is `INPUTVALUE`.
- `LET`, `ADD`, `SUB`, `MUL`, `DIV` a b: set a to b, a + b, a − b, a × b, a ÷ b (whole numbers, towards 0; dividing by 0 gives 0). a can be a register or one of the values that can be set (below).
- `CHANGEMAP x y tile, …`: each square x, y gets the tile: an over tile (0 to 52, or 100) as what stands there, a ground tile (53 to 100) as its ground; −1 clears the ground (the void), −2 clears what stands there, −3 both. A box at home that is replaced counts as gone from home. A ground made an arrow takes part in arrows from then on (section 4). The tile rules are applied to the square and its last pushed cleared. Tiles outside −3 to 100 do nothing. (Asking for a box at home, 33, 41 or 49, makes the original stop reading the command; none of Abe's levels do it: place the plain box, and the tile rules make it at home if it is on its target.)
- `SWAPOVER x1 y1 x2 y2` swaps what stands on the two squares, and their last pushed; if the player is on one of them, they move to the other. `SWAPUNDER` swaps the grounds (and last pushed). Both apply the tile rules to both squares, and do nothing unless both squares are inside 1 to 21.
- `GOTO x1 y1 x2 y2` goes on with the script of square x1, y1 (from its start, not coming back), as if run at x2, y2. If that square has no script, or x1, y1 isn't inside 1 to 21, the script ends.
- `POPUP` puts the plate at the script's square up.
- `TARGET` lets the player point at a square (with the mouse, or moving a pointer with the arrow keys, and clicking or pressing Return): its x and y go in `XPOS` and `YPOS`, or 0, 0 if it isn't in the pit.
- `REDGEMOFF`, `GREENGEMOFF`, `BLUEGEMOFF`: section 6.

### Values

- A number, −100 to 100.
- `R0` to `R9`: the script registers.
- The game's values, which can be read and set: `XGUY`, `YGUY` (the player's position: setting them moves the player there), `LASTX`, `LASTY` (where they were before their last move), `FACING`, `REDGEMS`, `GREENGEMS`, `BLUEGEMS`, `YELLOWGEMS` (−1 when carried), `REDKEYS`, `GREENKEYS`, `BLUEKEYS`, `INPUTVALUE`, `XPOS`, `YPOS`, `MOVES`, `HECTOMOVES`.
- `WALKABLE(tile)`: whether the tile can be walked on (1 or 0); setting it changes that for the rest of the level. (A tile below 0 counts as 0.)
- `DELTA(a, b)`: a + b (setting it does nothing).
- `UNDER(x, y)`, `OVER(x, y)`: the ground and what stands at x, y (x and y kept within 1 to 21). `OVER` on the player's square is 100 (unless they are a ghost and the level has no `GreenGem` script). Setting either sets that layer (kept within 0 to 100), clears its last pushed, and applies the tile rules; a ground set to an arrow takes part in arrows.
- `LASTPUSHED(x, y)`: the square's last pushed, which can be set.
- `REDGEMON`, `GREENGEMON`, `BLUEGEMON`: whether the gem is in use (the green gem's value is its stage: 1 just put in use, up to 7 a ghost; with a `GreenGem` script it stays 1).

All values are 16-bit signed whole numbers.

### When scripts run

- `Intro`, switched on, once when the level starts (before the first turn). Then `Master`, switched on, and what the player stands on is picked up. Restarting a level the player has made moves in doesn't show `Intro` again.
- `Master`, each turn: switched on at the start of the checks, switched off at the end.
- A square's script: for a plate when it is pressed or released (section 4), a wall with text when the player walks into it, a switch when it is flipped, a lock when a key opens it.
- The gem scripts: section 6, and each turn while the gem is in use (switched on).

## 8. Drawing

The game shows the pit around the player: in the original 9 × 7 squares with the player on the fifth square across and the fourth down (a bigger view shows more, with the player in the middle). Squares outside the pit (outside 0 to 21) are drawn as the void's ground (shape 0).

Each square in view is drawn in this order, all with the square's top left as the shape's top left:

1. Its ground: the shape with its ground's number. Water, lava and lava cooling are animated: the shape is the ground's number plus a frame, 0 to 3, which goes up by one with each square drawn (across the view, row by row) and wraps; so each square is a different frame, and the frames shift each time the pit is drawn. Glass sparkles: one square of the view at a time, the shape after glass (72, then 73, 74...) is shown with the same frame count; which square sparkles moves on through the view by 17 squares at a time.
2. Through their masks, the parts of the objects that reach into it: an object is drawn over two by two squares, its own square and the ones to its left, above, and above left. So on this square come: the object here (its shape + 3, its bottom right quarter), the object to the right (+2), the object below (+1), the object below right (+0). Objects further down and right are drawn over those further up and left.
3. After drawing, lava cooling moves on: where it was drawn with frame 1, the first step becomes the second and the second becomes lava. (So how fast lava cools depends on the drawing; both steps act as lava, so it doesn't change play, and an engine that draws differently may cool at another pace.)

The player is drawn as an object on their square (in the second step, as everything else), with these shapes:

- Standing or walking: 100 + facing (0, 4, 8, 12), + 0, 16, 0, 32 for the four steps of walking (the step goes on with each move, and is 0 again when they haven't moved for 90 ticks).
- Pushing (and no gem in use): 196 + facing + 0, 16, 0, 16 by the step; and facing east or south, the player's front quarters are drawn again over the box they push, with the parts of the objects below and to the right of them over that.
- A gem in use: 148 + facing + the step.
- The green gem's change: 228 + (stage − 2) × 4, for stages 2 to 6; as a ghost (stage 7) the player's four quarters are drawn last, over everything, at their square and the ones left, above and above left.

## 9. Sounds

| Sound | When |
|---|---|
| *crack* | a chest breaks to glass |
| *glass* | an object slides on glass |
| *lava* | a barrel or crate burns |
| *magic* | an arrow moves an object; a gem is put in use; `APPEAR` |
| *plate on* | a plate is pressed; a switch is flipped on |
| *plate off* | a switch is flipped off; `POPUP` |
| *scrape* | a box is pushed |
| *splinter* | a barrel or crate goes into a hole or wood |
| *water* | a barrel floats or a crate sinks |

Each is played once a turn at most, after the pit is drawn.
