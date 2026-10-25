# The Puzzle Pits level format (.PIT)

Each level is one file in `LEVELS/`, named `LEVELnn.PIT`. This describes the files as the game reads them (`GameLoadLevel` in PITS.C, `ReadString` and `ReadRun` in FILE.C and PITS.C). `tools/pit.mjs` reads and writes them. Its test (`node tools/pit_test.mjs`) checks that all forty of Abe's levels, and every script in them, come back byte for byte.

## The file

Every number is a 16-bit word, high byte first, unless it says otherwise.

| What | How |
|---|---|
| magic | the word `0x0895` |
| password | a string |
| title | a string, shown under the pit |
| floor | a map (the *under* layer) |
| objects | a map (the *over* layer) |
| number of scripts | a word; if 0, the file ends here |
| script map | a map: on each square, the number of the script that runs there, or 255 for none |
| scripts | for each one: its number (a word), its name (a string), its size (a word) and where its code starts (a word, from the start of the code) |
| code length | a word |
| code | that many bytes, XOR'd with `Amanda` (repeated) |

A script's size is not used when the game plays, except that a size of 0 means it has no code. In Abe's early levels the size is 5 more than the code's length, and in the later ones it's exact. Scripts are stored one after another, each ending with `END` (0). Some names have a meaning to the game:

- `Intro` runs as the level starts.
- `Master` runs after every move.
- `RedGem`, `GreenGem` and `BlueGem` run when a gem's power is used.

### Strings

A string is a word, its length XOR 22, followed by that many bytes XOR'd with `Mindy` (repeated).

### Maps

A pit is 23 by 23 squares, and the ones along its edge are always empty. A map stores the inner squares in the game's order: the array is `map[x][y]`, read as 529 cells in a row, starting at cell 24 and skipping the two edge cells at the end of each row of 23. The squares are run-length coded as pairs of bytes: a count (1 to 255), then the tile. A run can go on past the end of a row. The map ends once cell 505 is reached.

### Tiles

The *over* layer holds what stands on a square; the *under* layer holds the ground. Objects come in fours, for their frames or turns (`BARREL` is 29 to 32, for example).

| Over | | Under | |
|---|---|---|---|
| 0 | nothing | 53 | floor |
| 1 | wall | 54, 55, 56 | red, green, blue target |
| 5 | wall with text | 57 | marked tile |
| 9 | pillar | 58, 59 | plate, up and down |
| 13, 17 | switch, off and on | 60 to 63 | arrows: up, right, down, left |
| 21 | barrel of water | 64 to 67 | gems: red, green, blue, yellow |
| 25 | barrel with a hole | 68 to 70 | keys: red, green, blue |
| 29 | barrel | 71 | glass |
| 33 | blue barrel | 76 to 78 | locks: red, green, blue |
| 37 | crate | 79, 80 | crate of water, crate with a hole |
| 41 | red crate | 81 | wood |
| 45 | chest | 82 | grass |
| 49 | green chest | 83 | hole |
| 100 | the player | 84 | water |
| | | 88 | lava |
| | | 92, 96 | lava cooling |

`CHANGEMAP` also takes −3 (clear both layers), −2 (clear the object) and −1 (clear the ground).

## Scripts

The code is a sequence of commands, interpreted by `RunPitCode` in PITS.C. Each command is one byte, followed by its values and, for blocks, a jump. `tools/pit.mjs` shows them as text, one command a line:

```
SWITCHON
  IF EQ OVER(XPOS, YPOS) 29
    PRINT "Flosh speaks: ..."
    CHANGEMAP 5 2 67
    LET R2 1
  ELSE
    ...
  ENDIF
SWITCHOFF
  POPUP
```

### Values

A value is one byte, sometimes followed by more values:

| Byte | Value |
|---|---|
| 0 to 100 | that number |
| 101 to 200 | −100 to −1 (the byte minus 201) |
| 201 to 210 | the script registers `R0` to `R9` |
| 211 to 227 | the game's registers: `XGUY`, `YGUY` (the player), `LASTX`, `LASTY`, `FACING`, `REDGEMS`, `GREENGEMS`, `BLUEGEMS`, `YELLOWGEMS`, `REDKEYS`, `GREENKEYS`, `BLUEKEYS`, `INPUTVALUE`, `XPOS`, `YPOS` (the script's square), `MOVES`, `HECTOMOVES` |
| 228 | `WALKABLE(tile)`: whether a tile can be walked on |
| 229 | `DELTA(a, b)`: a + b |
| 230 | `UNDER(x, y)`: the ground at x, y |
| 231 | `OVER(x, y)`: what stands at x, y (100 if it's the player) |
| 232 | `LASTPUSHED(x, y)` |
| 233 to 235 | `REDGEMON`, `GREENGEMON`, `BLUEGEMON`: whether a gem's power is on |

### Commands

| Byte | Command | Then |
|---|---|---|
| 0 | `END` | the end of the script |
| 1 | `ENDIF` | |
| 2 | `NEXT` | |
| 3 | `ELSE` | a jump to its `ENDIF` |
| 4 | `SWITCHOFF` | what follows runs when the square is switched off (left) |
| 5 | `SWITCHON` | a jump to its `SWITCHOFF`: what follows runs when it's switched on (stepped on) |
| 6 to 11 | `IF EQ`, `LT`, `GT`, `LE`, `GE`, `NE` | two values, a jump to its `ELSE` or `ENDIF` |
| 12 | `IF INPUT "text"` | the text's length, a jump, the text: whether the player typed it |
| 13 | `FOR register from to` | three values, a jump past its `NEXT`; it counts up or down |
| 14 | `REDRAW` | |
| 15 | `APPEAR` | redraw, with the magic sound and a fade |
| 16 | `STOP` | the end of the script, for now |
| 17 | `WIN` | the level is won |
| 18 | `TARGET` | the player points at a square, which goes in `XPOS`, `YPOS` |
| 19 to 21 | `REDGEMOFF`, `GREENGEMOFF`, `BLUEGEMOFF` | |
| 22 | `POPUP` | the plate here pops up |
| 23 to 27 | `LET`, `ADD`, `SUB`, `MUL`, `DIV` | a register (or lookup) and a value: `ADD R1 2` is R1 = R1 + 2 |
| 28, 29 | `SWAPOVER`, `SWAPUNDER` | x1 y1 x2 y2: swap two squares' objects or ground |
| 30 | `GOTO` | x1 y1 x2 y2: run the script on square x1, y1, as if at x2, y2 |
| 31 | `CHANGEMAP` | the length of what follows, then sets of x, y, tile: `CHANGEMAP 5 2 67, 6 2 53` |
| 32 | `PRINT "text"` | its length, then the text, shown on the left (`\n` for a new line) |
| 33 | `INPUT "text"` | the same, then asks the player to type something |

The jumps are 16-bit offsets counted from where the jump itself is stored. An `IF` jumps to its own `ELSE` or `ENDIF`, an `ELSE` to its `ENDIF`, a `FOR` to just past its `NEXT`, and a `SWITCHON` to its `SWITCHOFF`. The text form has no jumps at all: the assembler works them out from the blocks, the same way Abe's compiler did.
