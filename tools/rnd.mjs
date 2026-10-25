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
	const w = Math.max(lw, RND_VIEW), h = Math.max(lines.length, RND_VIEW);
	const left = (w - lw) >> 1, top = (h - lines.length) >> 1;
	const rows = Array.from({ length: h }, (_, y) => {
		const r = lines[y - top] ?? "";
		return ("_".repeat(left) + r).padEnd(w, "_").replace(/ /g, (m, i) => (i < left ? "_" : m));
	});
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
	chunk("INFO", [0x01, 1, 0x41, w >> 8, w & 255, 0x42, h >> 8, h & 255, 0x43, 0, 0, 0x44, 0, 0, 0x09, 1]); // RND engine, size, no time limit, no gems, auto-exit Sokoban
	const body = new DataView(new ArrayBuffer(w * h * 2));
	rows.forEach((r, y) => { for (let x = 0; x < w; x++) body.setUint16((y * w + x) * 2, RND_ELEMENT[r[x] ?? "_"] ?? 62); });
	chunk("BODY", new Uint8Array(body.buffer));
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
		if (r.frames.length > 1) conf += `${pad(r.name + ".delay:", 33)}4\n`;
		conf += "\n";
	});
	// where each character's picture is, for previews
	const at = { " ": "empty_space", "#": "steelwall", "_": "invisible_steelwall", "$": "sokoban_object", ".": "sokoban_field_empty", "*": "sokoban_field_full", "@": "player_1", "+": "sokoban_field_player" };
	for (const { char, n } of Object.values(RND_DECOR)) at[char] = `empty_space_${n}`;
	const where = Object.fromEntries(Object.entries(at).map(([c, name]) => [c, rows.findIndex((r) => r.name === name) * T]));
	return { width, height, rgba, conf, tile: T, where };
}
