// The Puzzle Pits' levels for Rocks'n'Diamonds, shared by tools/pit2rnd.mjs
// and the level editor. Pits is Sokoban with more to it, so a level
// becomes a Sokoban level, in rows of Sokoban's characters (docs/RND-PORT.md):
//
//   #  wall          $  box         .  goal (a target)     @  player
//   _  the void      *  box on goal +  player on goal      (space) floor
//   m  marked tile   p  plate       g  grass (only shown, walked over)
//
// What Sokoban has no place for is approximated, and reported: water, lava
// and holes become walls (the player can't go there in Pits either, but
// objects could be pushed in), and keys, locks, arrows, gems, plates and
// switches become floor. The stories, in the scripts, are left out.
import { disassemble, SIZE } from "./pit.mjs";
import { blit } from "./tiles.mjs";

const RND_WALLS = new Set([1, 5, 9, 13, 17, 21, 25]); // walls, pillar, switches, barrels stuck in water or a hole
const RND_BOXES = new Set([29, 37, 45]); // barrel, crate, chest
const RND_FULL = new Set([33, 41, 49]); // each on its target
const RND_GOALS = new Set([54, 55, 56]);
const RND_BLOCKED = new Set([0, 83, 84, 88, 92, 96]); // void, hole, water, lava (and cooling)
const RND_KINDS = { 29: "barrel", 33: "barrel", 37: "crate", 41: "crate", 45: "chest", 49: "chest" };
// ground with no part in a Sokoban level, shown as it is: R'n'D's
// decorative empty spaces (EL_EMPTY_SPACE_1 is 1217, src/conf_emp.h)
const RND_DECOR = { 57: { char: "m", n: 1 }, 58: { char: "p", n: 2 }, 82: { char: "g", n: 3 } };

// a level as rows of Sokoban, and what it loses
export function rndConvert(level) {
	const lost = new Set();
	const grid = [];
	let goals = 0;
	for (let y = 1; y < SIZE - 1; y++) {
		let row = "";
		for (let x = 1; x < SIZE - 1; x++) {
			let o = level.over[x][y], u = level.under[x][y];
			if (o > 0 && o < 100) o -= (o - 1) % 4; // the first of its four
			const goal = RND_GOALS.has(u);
			if (goal) goals++;
			let c;
			if (o === 100) c = goal ? "+" : "@";
			else if (RND_FULL.has(o)) c = "*";
			else if (RND_BOXES.has(o)) c = goal ? "*" : "$";
			else if (RND_WALLS.has(o)) c = "#";
			else if (u === 0) c = "_";
			else if (RND_BLOCKED.has(u)) { c = "#"; lost.add({ 83: "holes", 84: "water", 88: "lava", 92: "lava", 96: "lava" }[u]); }
			else c = goal ? "." : RND_DECOR[u === 59 ? 58 : u]?.char ?? " ";
			if (o === 21 || o === 25) lost.add("barrels in water or holes");
			if (u >= 60 && u <= 63) lost.add("arrows");
			if (u >= 64 && u <= 67) lost.add("gems");
			if (u >= 68 && u <= 70 || u >= 76 && u <= 78) lost.add("keys and locks");
			if (u === 58 || u === 59 || o === 13 || o === 17 || o === 5) lost.add("plates, switches and signs");
			if (u === 71) lost.add("glass");
			if (u === 81) lost.add("wood");
			row += c;
		}
		grid.push(row);
	}
	const kinds = new Set();
	for (let x = 0; x < SIZE; x++) for (let y = 0; y < SIZE; y++) if (RND_KINDS[level.over[x][y]]) kinds.add(RND_KINDS[level.over[x][y]]);
	if (kinds.size > 1) lost.add(`colours (${[...kinds].join(", ")} in one)`);
	// Pits is won when every target holds its colour; a script can win it
	// too, which matters when there are no targets
	if (!goals && level.scripts.some((s) => s.size && disassemble(level.code, s.offset).lines.some((l) => l.trim() === "WIN"))) lost.add("how it's won (its scripts)");
	// only the part with something in it
	const used = (r) => /[^_]/.test(r);
	while (grid.length && !used(grid[0])) grid.shift();
	while (grid.length && !used(grid.at(-1))) grid.pop();
	const left = Math.min(...grid.map((r) => r.search(/[^_]/)));
	const right = Math.max(...grid.map((r) => r.length - r.split("").reverse().join("").search(/[^_]/)));
	return { rows: grid.map((r) => r.slice(left, right).replace(/_+$/, "")), lost: [...lost] };
}

// the rows as plain Sokoban (.xsb), for other Sokoban games
export function rndXsb(rows) { return rows.map((l) => l.replace(/[mpg]/g, " ")).join("\n"); }

// ---- Rocks'n'Diamonds' own level files (src/files.c SaveLevelFromFilename):
// chunks VERS, DATE, NAME, AUTH, INFO (typed values) and BODY (16 bits a
// square)

const RND_ELEMENT = { ...Object.fromEntries(Object.values(RND_DECOR).map(({ char, n }) => [char, 1216 + n])), " ": 0, "#": 13, "$": 67, ".": 68, "*": 69, "@": 80, "+": 63, "_": 62 }; // EL_ numbers (src/main.h)
// how many squares the playfield shows (rndArtwork): a smaller level is
// padded with the void to fill it, as R'n'D draws what is beyond a
// level's edge as floor
const RND_VIEW = 11;

export function rndLevelFile(title, author, lines) {
	const lw = Math.max(...lines.map((r) => r.length));
	const left = (Math.max(lw, RND_VIEW) - lw) >> 1;
	const grid = lines.map((r) => [...("_".repeat(left) + r).replace(/ /g, (m, i) => (i < left ? "_" : m))].map((c) => RND_ELEMENT[c] ?? 62));
	return levelFile(title, author, grid, [0x09, 1]); // auto-exit Sokoban
}

// a level with Pits' rules (rndRules, rndCells); rows of Sokoban when its
// conversion was corrected by hand
export function rndPitsLevelFile(title, author, level) {
	const { grid, left } = rndPitsGrid(level);
	const gems = [64, 66].some((u) => level.under.some((col) => col.includes(u)));
	return { bytes: levelFile(title, author, grid, [], rndRules(left ?? 0, gems), [0, 80, 0x11, 1]), scripted: left === null }; // the player keeps the floor it starts on
}

// the level's rows of elements, as rndPitsLevelFile writes them
export function rndPitsGrid(level) {
	const { cells, left } = rndCells(level);
	// rows of elements, only the part with something in it
	let grid = Array.from({ length: SIZE }, (_, y) => cells.map((col) => col[y]));
	const used = (e) => e !== 62;
	while (grid.length && !grid[0].some(used)) grid.shift();
	while (grid.length && !grid.at(-1).some(used)) grid.pop();
	const l = Math.min(...grid.map((r) => r.findIndex(used)).filter((i) => i >= 0));
	const r = Math.max(...grid.map((row) => row.length - [...row].reverse().findIndex(used)));
	grid = grid.map((row) => row.slice(l, r));
	const w = grid[0].length;
	grid = grid.map((row) => [...Array((Math.max(w, RND_VIEW) - w) >> 1).fill(62), ...row]);
	// the panel, a row above the pit: the gems' buttons (and the counter,
	// hidden in the void)
	const has = (u) => level.under.some((col) => col.includes(u));
	const panel = [...(left !== null ? [rndCe("counter")] : []), ...(has(64) ? [rndCe("red_gems_0")] : []), ...(has(66) ? [rndCe("blue_gems_0")] : [])];
	if (panel.length) grid.unshift(Array.from({ length: grid[0].length }, (_, x) => panel[x] ?? 62));
	return { grid, left };
}

// what a level with Pits' rules still loses
export function rndLost(level) {
	const lost = new Set();
	for (let x = 0; x < SIZE; x++) for (let y = 0; y < SIZE; y++) {
		const o = level.over[x][y], u = level.under[x][y];
		if (u === 65) lost.add("the green gem's power");
		if (u >= 68 && u <= 70 || u >= 76 && u <= 78) lost.add("keys and locks");
		if (u === 58 || u === 59 || (o >= 13 && o <= 20) || (o >= 5 && o <= 8)) lost.add("plates, switches and signs (their scripts)");
	}
	if (rndCells(level).left === null && level.scripts.some((s) => s.size && disassemble(level.code, s.offset).lines.some((l) => l.trim() === "WIN"))) lost.add("how it's won (its scripts)");
	return [...lost];
}

function levelFile(title, author, lines, info, rules = [], elem = []) {
	const lw = Math.max(...lines.map((r) => r.length));
	const w = Math.max(lw, RND_VIEW), h = Math.max(lines.length, RND_VIEW);
	const top = (h - lines.length) >> 1;
	const rows = Array.from({ length: h }, (_, y) => { const r = lines[y - top] ?? []; return Array.from({ length: w }, (_, x) => r[x] ?? 62); });
	const parts = [];
	const latin = (s, n = s.length) => Uint8Array.from({ length: n }, (_, i) => s.charCodeAt(i) & 255 || 0);
	const chunk = (id, bytes) => {
		const head = new DataView(new ArrayBuffer(8));
		latin(id).forEach((b, i) => head.setUint8(i, b));
		head.setUint32(4, bytes.length);
		parts.push(new Uint8Array(head.buffer), Uint8Array.from(bytes));
	};
	chunk("VERS", [3, 0, 0, 0, 4, 4, 2, 4]); // file version 3.0, game 4.4.2.4
	const d = new Date();
	chunk("DATE", [d.getFullYear() >> 8, d.getFullYear() & 255, d.getMonth() + 1, d.getDate()]);
	chunk("NAME", latin(title.slice(0, 32), 32));
	chunk("AUTH", latin(author.slice(0, 32), 32));
	// a type byte (its top bits: 0x00 one byte, 0x40 two) and the value
	chunk("INFO", [0x01, 1, 0x41, w >> 8, w & 255, 0x42, h >> 8, h & 255, 0x43, 0, 0, 0x44, 0, 0, ...info]); // RND engine, size, no time limit, no gems
	const body = new DataView(new ArrayBuffer(w * h * 2));
	rows.forEach((r, y) => { for (let x = 0; x < w; x++) body.setUint16((y * w + x) * 2, r[x]); });
	chunk("BODY", new Uint8Array(body.buffer));
	if (elem.length) chunk("ELEM", elem);
	for (const [id, bytes] of rules) chunk(id, bytes);
	// the player's step is R'n'D's normal one: a square in 8 frames, about
	// Pits' pace (a step of a whole square would repeat every frame a key is
	// held, far faster than Pits)
	const all = [latin("RND1\0\0\0\0CAVE"), ...parts];
	const out = new Uint8Array(all.reduce((a, p) => a + p.length, 0));
	let at = 0;
	for (const p of all) { out.set(p, at); at += p.length; }
	return out;
}

export function rndLevelinfo(name, author, levels) {
	return `file_identifier:                ROCKSNDIAMONDS_LEVELINFO_FILE_VERSION_3.0

name:                           ${name}
author:                         ${author}
levels:                         ${levels}
first_level:                    1
`;
}

// ---- the pictures: 48 pixels a square, Pits' 24 doubled (sharp); one
// row of the sheet a picture, its frames along it, and graphicsinfo.conf

export function rndArtwork(shapes) {
	const T = 48;
	const tile = () => new Uint8ClampedArray(T * T * 4);
	const ground = (n) => { // a ground tile, doubled
		const small = new Uint8ClampedArray(24 * 24 * 4);
		blit(small, 24, shapes[n], 0, 0, false);
		const t = tile();
		for (let y = 0; y < T; y++) for (let x = 0; x < T; x++) for (let k = 0; k < 4; k++) t[(y * T + x) * 4 + k] = small[((y >> 1) * 24 + (x >> 1)) * 4 + k];
		return t;
	};
	const quarters = (n) => { // an object as Pits draws it, over 48 by 48
		const c = new Uint8ClampedArray(48 * 48 * 4);
		[[0, 0, 0], [1, 24, 0], [2, 0, 24], [3, 24, 24]].forEach(([part, px, py]) => blit(c, 48, shapes[n + part], px, py, true));
		return c;
	};
	// crop: a 24-pixel window of an object, doubled, so it fills a square as
	// the ground does; at x, y, or round what the picture has (the player);
	// on a ground tile, if given; round: only the circle filling the square
	const crop = (n, under, x, y, round) => {
		const c = quarters(n);
		if (x === undefined) {
			let a = 48, b = 48, d = -1, e = -1;
			for (let j = 0; j < 48; j++) for (let i = 0; i < 48; i++) if (c[(j * 48 + i) * 4 + 3]) { a = Math.min(a, i); d = Math.max(d, i); b = Math.min(b, j); e = Math.max(e, j); }
			x = Math.round((a + d + 1) / 2 - 12);
			y = Math.round((b + e + 1) / 2 - 12);
		}
		x = Math.max(0, Math.min(24, x));
		y = Math.max(0, Math.min(24, y));
		const t = under === undefined ? tile() : ground(under);
		for (let j = 0; j < T; j++) for (let i = 0; i < T; i++) {
			const s = ((y + (j >> 1)) * 48 + x + (i >> 1)) * 4;
			const r = Math.hypot(i + 0.5 - T / 2, j + 0.5 - T / 2);
			if (round && r > T / 2) continue;
			if (round && r > T / 2 - 2) { t.set([0, 0, 0, 255], (j * T + i) * 4); continue; }
			if (c[s + 3]) for (let k = 0; k < 4; k++) t[(j * T + i) * 4 + k] = c[s + k];
		}
		return t;
	};
	// a barrel's lid, without its side; a wall's top face: seen from above
	const BARREL_X = 15, BARREL_Y = 17;
	const wall = () => crop(1, undefined, 12, 12);

	const rows = [];
	const add = (name, frames) => rows.push({ name, frames });
	add("empty_space", [ground(53)]);
	for (const [u, { n }] of Object.entries(RND_DECOR)) add(`empty_space_${n}`, [ground(+u)]);
	add("steelwall", [wall()]);
	// the void (Sokoban's _, around and in the pits): black, over the ground
	add("invisible_steelwall", [(() => { const t = tile(); for (let i = 3; i < t.length; i += 4) t[i] = 255; return t; })()]);
	add("sokoban_object", [crop(29, undefined, BARREL_X, BARREL_Y, true)]); // see-through: R'n'D draws the ground under what moves
	add("sokoban_field_empty", [ground(56)]);
	add("sokoban_field_full", [crop(33, 56, BARREL_X, BARREL_Y, true)]);
	for (const [dir, facing] of Object.entries({ up: 0, right: 4, down: 8, left: 12 })) {
		add(`player_1.${dir}`, [crop(100 + facing)]);
		add(`player_1.moving.${dir}`, [0, 16, 0, 32].map((f) => crop(100 + facing + f)));
		add(`player_1.pushing.${dir}`, [0, 16].map((f) => crop(196 + facing + f)));
	}
	add("player_1", [crop(100 + 8)]);
	// Pits' rules: the grounds (water and lava moving), the boxes on them
	function picture(ce) {
		if (ce === "start") return [ground(53)];
		if (ALIAS[ce]) return picture(ALIAS[ce]);
		if (BUTTONS.includes(ce)) { // the gem, and a dot for each one there is (dim when none)
			const [c, , n] = ce.split("_"), t = ground(GROUND_TILES[`${c}_gem`]);
			for (let i = 0; i < t.length; i += 4) if (+n === 0) for (let k = 0; k < 3; k++) t[i + k] >>= 2;
			for (let k = 0; k < +n; k++) for (let y = 40; y < 45; y++) for (let x = 2 + k * 6; x < 6 + k * 6; x++) t.set([255, 255, 255, 255], (y * T + x) * 4);
			return [t];
		}
		if (ce === "counter") return [(() => { const t = tile(); for (let i = 3; i < t.length; i += 4) t[i] = 255; return t; })()];
		if (GROUND_TILES[ce] !== undefined) return ce === "water" || ce === "lava" ? [0, 1, 2, 3].map((f) => ground(GROUND_TILES[ce] + f)) : [ground(GROUND_TILES[ce])];
		const [kind, at] = ce.split("@"), g = at.split(">")[0], o = RND_OBJECTS[kind], under = GROUND_TILES[g];
		const n = g === o.own ? o.home : g === "water" && o.water ? o.water : (g === "hole" || g === "wood") && o.hole ? o.hole : o.tile;
		return [kind === "barrel" ? crop(n, under, BARREL_X, BARREL_Y, true) : kind === "crate" ? crop(n, under, 12, 12) : crop(n, under, 14, 14)];
	}
	RND_CE_NAMES.forEach((ce, i) => add(`custom_${i + 1}`, picture(ce)));
	add("sokoban_field_player", [crop(100 + 8, 56)]);

	const width = T * Math.max(...rows.map((r) => r.frames.length)), height = T * rows.length;
	const rgba = new Uint8ClampedArray(width * height * 4);
	rows.forEach((r, j) => r.frames.forEach((f, i) => {
		for (let y = 0; y < T; y++) rgba.set(f.subarray(y * T * 4, (y + 1) * T * 4), ((j * T + y) * width + i * T) * 4);
	}));
	// the ground drawn under everything, standing or moving (the pictures of
	// what moves are see-through around it), and a playfield of whole
	// squares (RND_VIEW of 48 inside its 2-pixel border), in the middle of
	// where the standard one is
	const pad = (s, n) => s + " ".repeat(Math.max(1, n - s.length));
	let conf = "game.tile_size:                 48\ngame.use_masked_elements:       true\n" +
		"viewport.playfield.x:           14\nviewport.playfield.y:           14\nviewport.playfield.width:       532\nviewport.playfield.height:      532\n\n";
	rows.forEach((r, j) => {
		conf += `${pad(r.name + ":", 33)}pits.png\n${pad(r.name + ".xpos:", 33)}0\n${pad(r.name + ".ypos:", 33)}${j}\n${pad(r.name + ".frames:", 33)}${r.frames.length}\n${pad(r.name + ".tile_size:", 33)}${T}\n`;
		if (r.frames.length > 1) conf += `${pad(r.name + ".delay:", 33)}${r.name.startsWith("custom_") ? 8 : 4}\n`;
		conf += "\n";
	});
	// where each character's picture is, for previews
	const at = { " ": "empty_space", "#": "steelwall", "_": "invisible_steelwall", "$": "sokoban_object", ".": "sokoban_field_empty", "*": "sokoban_field_full", "@": "player_1", "+": "sokoban_field_player" };
	for (const { char, n } of Object.values(RND_DECOR)) at[char] = `empty_space_${n}`;
	const where = Object.fromEntries(Object.entries(at).map(([c, name]) => [c, rows.findIndex((r) => r.name === name) * T]));
	// and of each element in a level with Pits' rules
	Object.assign(where, { 0: where[" "], 13: where["#"], 62: where["_"] });
	RND_CE_NAMES.forEach((ce, i) => (where[EL_CUSTOM_1 + i] = rows.findIndex((r) => r.name === `custom_${i + 1}`) * T));
	return { width, height, rgba, conf, tile: T, where };
}

// ---- Pits' own rules, as custom elements (docs/RND-PORT.md)
//
// Each ground is a custom element the player walks over (or not: water,
// lava, holes); a box is one for each ground it can stand on ("barrel on
// a blue target"), drawn with that ground under it. A box can enter any
// ground but grass; it leaves its ground behind, and becomes the box on
// the ground it entered ("digging" it, in R'n'D's words), which can move
// by itself (arrows), slide (glass), or be gone (lava; a crate in water
// sinks to a bridge). A counter, hidden in the void, ends the level when
// every target holds its colour. The player starts on a floor element
// that turns into the player.

const EP = { DIGGABLE: 0, INDESTRUCTIBLE: 5, CAN_CHANGE: 7, CAN_MOVE: 8, WALKABLE_OVER: 16, PUSHABLE: 24, CAN_MOVE_INTO_ACID: 27 };
const CE_EV = { DELAY: 0, PRESSED_BY_PLAYER: 2, PUSHED_BY_PLAYER: 3, CHANGE_OF_X: 9, VALUE_GETS_ZERO: 16, BY_OTHER_ACTION: 18, ENTERED_BY_PLAYER: 21, LEFT_BY_PLAYER: 22, DIGGING_X: 36, CREATION_OF_X: 37, CLICKED_BY_MOUSE: 44, NEXT_TO_X: 49 };
const TRIGGERED = new Set([CE_EV.CHANGE_OF_X, CE_EV.CREATION_OF_X, CE_EV.NEXT_TO_X]); // what another element does: R'n'D wants "by other action" with it
const MV = { LEFT: 1, RIGHT: 2, UP: 4, DOWN: 8, ALL: 15, PREVIOUS: 1 << 4, WHEN_PUSHED: 1 << 10 };
const CA = { EXIT_PLAYER: 1, SET_CE_VALUE: 16 }, CA_MODE = { SET: 1, ADD: 2, SUBTRACT: 3 }, CA_PLAYER_ANY = 10015;
const EL_PLAYER_1 = 80, EL_CUSTOM_1 = 360, EL_GROUP_1 = 624;

const GROUND_TILES = {
	floor: 53, mark: 57, plate: 58, red_target: 54, green_target: 55, blue_target: 56,
	arrow_up: 60, arrow_right: 61, arrow_down: 62, arrow_left: 63, glass: 71, wood: 81, grass: 82,
	bridge: 79, hole_crate: 80, hole: 83, water: 84, lava: 88,
	red_gem: 64, green_gem: 65, blue_gem: 66, yellow_gem: 67, red_key: 68, green_key: 69, blue_key: 70,
	red_lock: 76, green_lock: 77, blue_lock: 78,
};
const NOT_WALKABLE = new Set(["hole", "water", "lava"]);
const RND_OBJECTS = { barrel: { tile: 29, own: "blue_target", home: 33, water: 21, hole: 25 }, crate: { tile: 37, own: "red_target", home: 41 }, chest: { tile: 45, own: "green_target", home: 49 } };
const ARROWS = { arrow_up: MV.UP, arrow_right: MV.RIGHT, arrow_down: MV.DOWN, arrow_left: MV.LEFT };

// a box entering a ground: the ground it leaves there if the box is gone,
// or null: it can't go there (else it stays, as the box on that ground)
function boxGone(kind, ground) {
	if (kind === "crate" && ground === "water") return "bridge";
	if (ground === "lava") return kind === "chest" ? "glass" : "lava";
	if (kind !== "barrel" && ground === "hole") return "hole";
	if (kind === "crate" && ground === "wood") return "hole";
	return null;
}
const canEnter = (kind, ground) => ground !== "grass";

// the gems: collected by walking over them (so are keys); a red or blue
// one is used by clicking its button, beside the pit as in Pits' panel
// (one element for each count, as a change can't look at a value). Red:
// the floor and arrows are armed, and the one the player leaves becomes
// an arrow the way they went. Blue: the floor is armed, and the one left
// becomes a magnet for a moment, which a box behind it moves onto.
const COLLECTED = ["red_gem", "green_gem", "blue_gem", "yellow_gem", "red_key", "green_key", "blue_key"];
const MAX_GEMS = 8;
const POWERS = ["red", "blue"];
const SIDES = { up: 4, right: 2, down: 8, left: 1 }; // R'n'D's sides, the same as its directions
const OPPOSITE = { up: "down", down: "up", left: "right", right: "left" };
// grounds that look and act as another (box states, pictures)
const ALIAS = {
	"floor~red": "floor", ...Object.fromEntries(Object.keys(ARROWS).map((a) => [`${a}~red`, a])), "floor~blue": "floor",
	...Object.fromEntries(Object.keys(SIDES).map((d) => [`magnet_${d}`, "floor"])),
};
const ALL_GROUNDS = [...Object.keys(GROUND_TILES), "start", ...Object.keys(ALIAS)];
const baseOf = (g) => (g === "start" ? "floor" : ALIAS[g] ?? g);
const BUTTONS = POWERS.flatMap((c) => Array.from({ length: MAX_GEMS + 1 }, (_, n) => `${c}_gems_${n}`));
// and on glass, sliding on the way it came (glass>right): R'n'D's change
// knows the side a box came in by, not its direction once it is changed
const SLIDES = { 1: "right", 2: "left", 4: "down", 8: "up" }; // the side it came in by: the way it goes
// and on the floor, pulled a square (floor>pull_up) by a blue gem's magnet
const BOXES = Object.keys(RND_OBJECTS).flatMap((k) => [...Object.keys(GROUND_TILES).filter((g) => canEnter(k, g) && !boxGone(k, g)), ...Object.values(SLIDES).map((d) => `glass>${d}`), ...Object.keys(SIDES).map((d) => `floor>pull_${d}`)].map((g) => `${k}@${g}`));

// custom_1, custom_2 ... in this order, the same in every level, so one
// artwork set draws them all
export const RND_CE_NAMES = [...Object.keys(GROUND_TILES), "start", ...BOXES, "counter", ...Object.keys(ALIAS), ...BUTTONS];
export const rndCe = (name) => EL_CUSTOM_1 + RND_CE_NAMES.indexOf(name);
// groups of elements, at most 16 each (more are split into groups of groups)
const GROUP_LIST = [];
function defineGroup(name, members) {
	if (members.length <= 16) { GROUP_LIST.push([name, members]); return; }
	const parts = [];
	for (let i = 0; i < members.length; i += 16) { parts.push(`${name}#${i / 16}`); defineGroup(parts.at(-1), members.slice(i, i + 16)); }
	GROUP_LIST.push([name, parts.map((p) => ({ group: p }))]);
}
defineGroup("boxes_home", Object.entries(RND_OBJECTS).map(([k, o]) => `${k}@${o.own}`));
for (const k of Object.keys(RND_OBJECTS)) defineGroup(`enter_${k}`, ALL_GROUNDS.filter((g) => canEnter(k, baseOf(g))));
for (const c of POWERS) defineGroup(`${c}_buttons`, BUTTONS.filter((b) => b.startsWith(c + "_")));
defineGroup("red_armed", Object.keys(ALIAS).filter((g) => g.endsWith("~red")));
const rndGroup = (name) => EL_GROUP_1 + GROUP_LIST.findIndex(([n]) => n === name);

// the typed values R'n'D's level files keep (files.c: chunk_config_CUSX_*)
function micro(out, conf, value, size) {
	out.push(conf);
	if (size === 1) out.push(value & 255);
	else if (size === 2) out.push((value >> 8) & 255, value & 255);
	else out.push((value >>> 24) & 255, (value >> 16) & 255, (value >> 8) & 255, value & 255);
}
function customChunk(el, d) {
	const out = [el >> 8, el & 255];
	micro(out, 0x81, (d.props ?? []).reduce((a, p) => a | (1 << EP[p]), 1 << EP.CAN_MOVE_INTO_ACID) >>> 0, 4);
	if (d.moveEnter !== undefined) micro(out, 0x4c, d.moveEnter, 2);
	if (d.moveLeave !== undefined) micro(out, 0x4d, d.moveLeave, 2);
	if (d.movePattern !== undefined) micro(out, 0x83, d.movePattern, 4);
	if (d.moveDir !== undefined) micro(out, 0x04, d.moveDir, 1);
	if (d.moveDelay !== undefined) micro(out, 0x4a, d.moveDelay, 2);
	micro(out, 0x46, 0, 2); // pushed at once, as in Pits
	micro(out, 0x47, 0, 2);
	if (d.value !== undefined) micro(out, 0x44, d.value, 2);
	const pages = d.pages ?? [];
	micro(out, 0x09, Math.max(1, pages.length), 1); // the last of the element's own values
	if (!pages.length) micro(out, 0x01, 0, 1); // R'n'D reads a first page all the same
	pages.forEach((p, i) => {
		micro(out, 0x01, i, 1);
		const bits = [0, 0];
		for (const e of [...p.events, ...(p.events.some((e) => TRIGGERED.has(e)) ? [CE_EV.BY_OTHER_ACTION] : [])]) bits[e >> 5] |= 1 << (e & 31);
		micro(out, 0x81, bits[0] >>> 0, 4);
		if (bits[1]) micro(out, 0x82, bits[1] >>> 0, 4);
		micro(out, 0x02, p.target !== undefined ? 1 : 0, 1);
		if (p.target !== undefined) micro(out, 0x41, p.target, 2);
		if (p.trigger !== undefined) micro(out, 0x45, p.trigger, 2);
		if (p.side !== undefined) micro(out, 0x04, p.side, 1);
		if (p.triggerPage !== undefined) micro(out, 0x83, p.triggerPage, 4);
		if (p.delay !== undefined) { micro(out, 0x42, p.delay, 2); micro(out, 0x44, 1, 2); } // delay_fixed of delay_frames: p.delay frames
		if (p.action) {
			micro(out, 0x0c, 1, 1);
			micro(out, 0x0d, p.action.type, 1);
			micro(out, 0x0e, p.action.mode ?? 0, 1);
			micro(out, 0x46, p.action.arg ?? 0, 2);
		}
	});
	return out;
}
function groupChunk(el, members) {
	const out = [el >> 8, el & 255, 0xc2, (members.length * 2) >> 8, (members.length * 2) & 255];
	for (const m of members) out.push(m >> 8, m & 255);
	return out;
}

// the chunks for a level with `left` targets still to fill
export function rndRules(left, gems = true) {
	const chunks = [];
	const custom = (name, d) => chunks.push(["CUSX", customChunk(rndCe(name), d)]);
	// (grounds aren't indestructible: a box couldn't enter them)
	const armed = (c) => ({ events: [CE_EV.CHANGE_OF_X], trigger: rndGroup(`${c}_buttons`), triggerPage: 1 }); // a button clicked (its page 0)
	for (const g of Object.keys(GROUND_TILES)) {
		const pages = [];
		if (COLLECTED.includes(g)) pages.push({ events: [CE_EV.ENTERED_BY_PLAYER], target: rndCe("floor") });
		if (g === "floor" || ARROWS[g]) pages.push({ ...armed("red"), target: rndCe(`${g}~red`) });
		if (g === "floor") pages.push({ ...armed("blue"), target: rndCe("floor~blue") });
		custom(g, { props: NOT_WALKABLE.has(g) ? [] : ["WALKABLE_OVER", ...(pages.length ? ["CAN_CHANGE"] : [])], pages });
	}
	for (const g of Object.keys(ALIAS)) {
		const pages = [];
		if (g.endsWith("~red") || g === "floor~blue") {
			for (const [d, side] of Object.entries(SIDES)) pages.push({ events: [CE_EV.LEFT_BY_PLAYER], side, target: rndCe(g.endsWith("~red") ? `arrow_${d}` : `magnet_${d}`) });
			pages.push({ events: [CE_EV.CHANGE_OF_X], trigger: g.endsWith("~red") ? rndGroup("red_armed") : rndCe("floor~blue"), target: rndCe(ALIAS[g]) }); // one used: the rest as they were
		} else pages.push({ events: [CE_EV.DELAY], delay: 24, target: rndCe("floor") }); // a magnet, while the player steps off it and a box comes
		custom(g, { props: ["WALKABLE_OVER", "CAN_CHANGE"], pages });
	}
	// (without gems, no buttons: a level with mouse clicks waits for a key to start)
	if (gems) for (const b of BUTTONS) {
		const [c, , n] = b.split("_"), k = +n;
		custom(b, { props: ["INDESTRUCTIBLE", "CAN_CHANGE"], pages: [
			k > 0 ? { events: [CE_EV.CLICKED_BY_MOUSE], target: rndCe(`${c}_gems_${k - 1}`) } : { events: [] },
			...(k < MAX_GEMS ? [{ events: [CE_EV.CHANGE_OF_X], trigger: rndCe(`${c}_gem`), target: rndCe(`${c}_gems_${k + 1}`) }] : []),
		] });
	}
	// the start: a floor that turns into the player once (kept under them,
	// it would turn again at every delay; its value falls to 0 only once)
	custom("start", { props: ["WALKABLE_OVER", "CAN_CHANGE"], value: 1, pages: [
		{ events: [CE_EV.DELAY], delay: 1, action: { type: CA.SET_CE_VALUE, mode: CA_MODE.SET, arg: 0 } },
		{ events: [CE_EV.VALUE_GETS_ZERO], target: EL_PLAYER_1 },
	] });
	for (const name of BOXES) {
		const [kind, ground] = name.split("@");
		const [base, slide] = ground.split(">");
		const d = { props: ["PUSHABLE", "CAN_CHANGE"], moveEnter: rndGroup(`enter_${kind}`), moveLeave: rndCe(base), pages: [] };
		const way = ARROWS[base] ?? (slide && MV[slide.replace("pull_", "").toUpperCase()]);
		if (way) { Object.assign(d, { movePattern: way, moveDir: way, moveDelay: 0 }); d.props.push("CAN_MOVE"); }
		for (const g of ALL_GROUNDS) {
			const b = baseOf(g);
			if (!canEnter(kind, b)) continue;
			const sides = b === "glass" ? Object.entries(SLIDES) : [[undefined]];
			for (const [side, to] of sides) {
				const state = boxGone(kind, b) ?? `${kind}@${b}${to ? ">" + to : ""}`;
				if (state !== name) d.pages.push({ events: [CE_EV.DIGGING_X], trigger: rndCe(g), side: side && +side, target: rndCe(state) });
			}
		}
		// a magnet next to it, the way it would go: pulled onto it
		if (ground === "floor") for (const dir of Object.keys(SIDES)) d.pages.push({ events: [CE_EV.NEXT_TO_X], trigger: rndCe(`magnet_${dir}`), side: SIDES[OPPOSITE[dir]], target: rndCe(`${kind}@floor>pull_${dir}`) });
		custom(name, d);
	}
	// down when a box comes home, up when one leaves; at none, won
	custom("counter", {
		props: ["INDESTRUCTIBLE", "CAN_CHANGE"], value: left, pages: [
			{ events: [CE_EV.CREATION_OF_X], trigger: rndGroup("boxes_home"), action: { type: CA.SET_CE_VALUE, mode: CA_MODE.SUBTRACT, arg: 1 } },
			{ events: [CE_EV.CHANGE_OF_X], trigger: rndGroup("boxes_home"), action: { type: CA.SET_CE_VALUE, mode: CA_MODE.ADD, arg: 1 } },
			{ events: [CE_EV.VALUE_GETS_ZERO], action: { type: CA.EXIT_PLAYER, arg: CA_PLAYER_ANY } },
		],
	});
	for (const [name, members] of GROUP_LIST) chunks.push(["GRPX", groupChunk(rndGroup(name), members.map((m) => (m.group ? rndGroup(m.group) : rndCe(m))))]);
	return chunks;
}

// a level's squares as these elements: the grid (x, y), and how many
// targets are still to fill (no targets: null, it's won by its scripts)
const KIND_OF = { 29: "barrel", 33: "barrel", 21: "barrel", 25: "barrel", 37: "crate", 41: "crate", 45: "chest", 49: "chest" };
export function rndCells(level) {
	const name = Object.fromEntries(Object.entries(GROUND_TILES).map(([g, t]) => [t, g]));
	Object.assign(name, { 59: "plate", 92: "lava", 96: "lava" });
	const cells = [];
	let targets = 0, home = 0;
	for (let x = 0; x < SIZE; x++) {
		cells.push([]);
		for (let y = 0; y < SIZE; y++) {
			let o = level.over[x][y];
			const u = level.under[x][y];
			if (o > 0 && o < 100) o -= (o - 1) % 4;
			const g = name[u];
			if (g?.endsWith("_target")) targets++;
			let e;
			if (o === 100) e = rndCe("start");
			else if ([1, 5, 9, 13, 17].includes(o)) e = 13; // steel wall
			else if (KIND_OF[o] && g && BOXES.includes(`${KIND_OF[o]}@${g}`)) {
				e = rndCe(`${KIND_OF[o]}@${g}`);
				if (g === RND_OBJECTS[KIND_OF[o]].own) home++;
			} else if (g) e = rndCe(g);
			else e = 62; // the void
			cells[x].push(e);
		}
	}
	return { cells, left: targets ? targets - home : null };
}
