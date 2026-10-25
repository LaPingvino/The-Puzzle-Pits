// node tools/pit2rnd.mjs [LEVELS-folder] [out-folder]: The Puzzle Pits'
// levels as a Rocks'n'Diamonds level set, with the game's own pictures.
//
// Pits is Sokoban with more to it, so the levels become Rocks'n'Diamonds'
// Sokoban levels (one .xsb file): walls, barrels, crates and chests as
// boxes, the red, green and blue targets as goals, and the player. What
// Sokoban has no place for is approximated, and reported for each level:
// water, lava, holes and the void become walls (the player can't go there
// in Pits either, but objects could be pushed in), and keys, locks,
// arrows, gems and the scripts' plates and switches become floor. The
// stories, in the scripts, are left out.
import fs from "node:fs";
import zlib from "node:zlib";
import path from "node:path";
import { readPit, SIZE } from "./pit.mjs";
import { readShapes, blit } from "./tiles.mjs";

const root = new URL("../", import.meta.url).pathname;
const [levelsDir = path.join(root, "PITS95/LEVELS"), out = "the_puzzle_pits"] = process.argv.slice(2);
const data = path.join(root, "PITS95");

// ---- the levels

const WALLS = new Set([1, 5, 9, 13, 17, 21, 25]); // walls, pillar, switches, barrels stuck in water or a hole
const BOXES = new Set([29, 37, 45]); // barrel, crate, chest
const FULL = new Set([33, 41, 49]); // each on its target
const GOALS = new Set([54, 55, 56]);
const BLOCKED = new Set([0, 83, 84, 88, 92, 96]); // void, hole, water, lava (and cooling)
const PLAYER = 100;
const KINDS = { 29: "barrel", 33: "barrel", 37: "crate", 41: "crate", 45: "chest", 49: "chest" };

function toXsb(level) {
	const lost = new Set();
	const grid = [];
	for (let y = 1; y < SIZE - 1; y++) {
		let row = "";
		for (let x = 1; x < SIZE - 1; x++) {
			let o = level.over[x][y], u = level.under[x][y];
			if (o > 0 && o < PLAYER) o -= (o - 1) % 4; // the first of its four
			const goal = GOALS.has(u);
			let c;
			if (o === PLAYER) c = goal ? "+" : "@";
			else if (FULL.has(o)) c = "*";
			else if (BOXES.has(o)) c = goal ? "*" : "$";
			else if (WALLS.has(o)) c = "#";
			else if (u === 0) c = "_";
			else if (BLOCKED.has(u)) { c = "#"; lost.add({ 83: "holes", 84: "water", 88: "lava", 92: "lava", 96: "lava" }[u]); }
			else c = goal ? "." : " ";
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
	for (let x = 0; x < SIZE; x++) for (let y = 0; y < SIZE; y++) if (KINDS[level.over[x][y]]) kinds.add(KINDS[level.over[x][y]]);
	if (kinds.size > 1) lost.add(`colours (${[...kinds].join(", ")} in one)`);
	// only the part with something in it
	const used = (r) => /[^_]/.test(r);
	while (grid.length && !used(grid[0])) grid.shift();
	while (grid.length && !used(grid.at(-1))) grid.pop();
	let left = Math.min(...grid.map((r) => r.search(/[^_]/)));
	let right = Math.max(...grid.map((r) => r.length - r.split("").reverse().join("").search(/[^_]/)));
	return { rows: grid.map((r) => r.slice(left, right).replace(/_+$/, "")), lost: [...lost] };
}

// ---- the pictures: 48 pixels a square, Pits' 24 doubled (sharp), its
// objects (drawn over two by two squares in Pits) as they are

const shapes = readShapes(...["TILES1.DAT", "TILES2.DAT", "TILES3.DAT"].map((f) => new Uint8Array(fs.readFileSync(path.join(data, f)))));
const T = 48;

function tile() { return new Uint8ClampedArray(T * T * 4); }
function ground(n) { // a ground tile, doubled
	const small = new Uint8ClampedArray(24 * 24 * 4);
	blit(small, 24, shapes[n], 0, 0, false);
	const t = tile();
	for (let y = 0; y < T; y++) for (let x = 0; x < T; x++) for (let k = 0; k < 4; k++) t[(y * T + x) * 4 + k] = small[((y >> 1) * 24 + (x >> 1)) * 4 + k];
	return t;
}
function object(n, under) { // an object's four quarters, on a ground tile
	const t = under === undefined ? tile() : ground(under);
	[[0, 0, 0], [1, 24, 0], [2, 0, 24], [3, 24, 24]].forEach(([part, x, y]) => blit(t, T, shapes[n + part], x, y, true));
	return t;
}
function wall() { // a wall among walls: all its quarters in one square, doubled
	const small = new Uint8ClampedArray(24 * 24 * 4);
	for (let part = 0; part < 4; part++) blit(small, 24, shapes[1 + part], 0, 0, true);
	const t = tile();
	for (let y = 0; y < T; y++) for (let x = 0; x < T; x++) for (let k = 0; k < 4; k++) t[(y * T + x) * 4 + k] = small[((y >> 1) * 24 + (x >> 1)) * 4 + k];
	return t;
}

// the sheet: one row a picture, frames along it
const FLOOR = 53;
const rows = [];
const add = (name, frames) => { rows.push({ name, frames }); return rows.length - 1; };
add("empty_space", [ground(FLOOR)]);
add("steelwall", [wall()]);
// the void (Sokoban's _, around and in the pits): black, over the ground
add("invisible_steelwall", [(() => { const t = tile(); for (let i = 3; i < t.length; i += 4) t[i] = 255; return t; })()]);
add("sokoban_object", [object(29)]); // see-through: R'n'D draws the ground under what moves
add("sokoban_field_empty", [ground(56)]);
add("sokoban_field_full", [object(33, 56)]);
const dirs = { up: 0, right: 4, down: 8, left: 12 };
for (const [dir, facing] of Object.entries(dirs)) {
	add(`player_1.${dir}`, [object(100 + facing)]);
	add(`player_1.moving.${dir}`, [0, 16, 0, 32].map((f) => object(100 + facing + f)));
	add(`player_1.pushing.${dir}`, [0, 16].map((f) => object(196 + facing + f)));
}
add("player_1", [object(100 + 8)]);
add("sokoban_field_player", [object(100 + 8, 56)]);

const width = T * Math.max(...rows.map((r) => r.frames.length)), height = T * rows.length;
const sheet = new Uint8ClampedArray(width * height * 4);
rows.forEach((r, j) => r.frames.forEach((f, i) => {
	for (let y = 0; y < T; y++) sheet.set(f.subarray(y * T * 4, (y + 1) * T * 4), ((j * T + y) * width + i * T) * 4);
}));

function png(w, h, rgba) {
	const crcTable = Array.from({ length: 256 }, (_, n) => { let c = n; for (let k = 0; k < 8; k++) c = c & 1 ? 0xedb88320 ^ (c >>> 1) : c >>> 1; return c >>> 0; });
	const crc = (b) => { let c = 0xffffffff; for (const x of b) c = crcTable[(c ^ x) & 0xff] ^ (c >>> 8); return (c ^ 0xffffffff) >>> 0; };
	const chunk = (type, body) => {
		const len = Buffer.alloc(4); len.writeUInt32BE(body.length);
		const tb = Buffer.concat([Buffer.from(type), body]);
		const c = Buffer.alloc(4); c.writeUInt32BE(crc(tb));
		return Buffer.concat([len, tb, c]);
	};
	const head = Buffer.alloc(13); head.writeUInt32BE(w, 0); head.writeUInt32BE(h, 4); head[8] = 8; head[9] = 6;
	const raw = Buffer.alloc((w * 4 + 1) * h);
	for (let y = 0; y < h; y++) Buffer.from(rgba.buffer, y * w * 4, w * 4).copy(raw, y * (w * 4 + 1) + 1);
	return Buffer.concat([Buffer.from([137, 80, 78, 71, 13, 10, 26, 10]), chunk("IHDR", head), chunk("IDAT", zlib.deflateSync(raw)), chunk("IEND", Buffer.alloc(0))]);
}

// ---- Rocks'n'Diamonds' own level files (src/files.c SaveLevelFromFilename):
// chunks VERS, DATE, NAME, AUTH, INFO (typed values) and BODY (16 bits a
// square)

const ELEMENT = { " ": 0, "#": 13, "$": 67, ".": 68, "*": 69, "@": 80, "+": 63, "_": 62 }; // EL_ numbers (src/main.h)

// VIEW is how many squares the playfield shows (graphicsinfo.conf): a
// smaller level is padded with the void to fill it, as R'n'D draws what is
// beyond a level's edge as floor
const VIEW = 11;

function rndLevel(title, author, lines) {
	const lw = Math.max(...lines.map((r) => r.length));
	const w = Math.max(lw, VIEW), h = Math.max(lines.length, VIEW);
	const left = (w - lw) >> 1, top = (h - lines.length) >> 1;
	const rows = Array.from({ length: h }, (_, y) => {
		const r = lines[y - top] ?? "";
		return ("_".repeat(left) + r).padEnd(w, "_").replace(/ /g, (m, i) => (i < left ? "_" : m));
	});
	const chunks = [];
	const chunk = (id, bytes) => { const head = Buffer.alloc(8); head.write(id, 0, "latin1"); head.writeUInt32BE(bytes.length, 4); chunks.push(head, Buffer.from(bytes)); };
	const fixed = (s, n) => { const b = Buffer.alloc(n); b.write(s.slice(0, n), "latin1"); return b; };
	chunk("VERS", [3, 0, 0, 0, 4, 4, 2, 4]); // file version 3.0, game 4.4.2.4
	const d = new Date();
	chunk("DATE", [d.getFullYear() >> 8, d.getFullYear() & 255, d.getMonth() + 1, d.getDate()]);
	chunk("NAME", fixed(title, 32));
	chunk("AUTH", fixed(author, 32));
	// a type byte (its top bits: 0x00 one byte, 0x40 two) and the value
	chunk("INFO", [0x01, 1, 0x41, w >> 8, w & 255, 0x42, h >> 8, h & 255, 0x43, 0, 0, 0x44, 0, 0, 0x09, 1]); // RND engine, size, no time limit, no gems, auto-exit Sokoban
	const body = Buffer.alloc(w * h * 2);
	rows.forEach((r, y) => { for (let x = 0; x < w; x++) body.writeUInt16BE(ELEMENT[r[x] ?? "_"] ?? 62, (y * w + x) * 2); });
	chunk("BODY", body);
	// the player's step is R'n'D's normal one: a square in 8 frames, about
	// Pits' pace (a step of a whole square would repeat every frame a key is
	// held, far faster than Pits)
	return Buffer.concat([Buffer.from("RND1\0\0\0\0CAVE", "latin1"), ...chunks]);
}

// ---- the level set

const files = fs.readdirSync(levelsDir).filter((n) => /level\d+\.pit$/i.test(n)).sort();
let xsb = "; The Puzzle Pits, by Abe Pralle (Plasma Works), 1995: its levels as Sokoban\n\n";
const report = [], native = [];
files.forEach((name, i) => {
	const level = readPit(new Uint8Array(fs.readFileSync(path.join(levelsDir, name))));
	const { rows: r, lost } = toXsb(level);
	xsb += `; ${i + 1}\n'${level.title}'\n${r.join("\n")}\n\n`;
	native.push(rndLevel(level.title, "Abe Pralle", r));
	report.push(`${String(i + 1).padStart(2)} ${level.title.padEnd(26)} ${lost.length ? lost.join(", ") : "as in Pits"}`);
});

fs.mkdirSync(path.join(out, "graphics"), { recursive: true });
native.forEach((b, i) => fs.writeFileSync(path.join(out, `${String(i + 1).padStart(3, "0")}.level`), b));
fs.writeFileSync(path.join(out, "levels.xsb"), xsb); // the same levels as Sokoban text, for other Sokoban games
fs.writeFileSync(path.join(out, "levelinfo.conf"), `file_identifier:                ROCKSNDIAMONDS_LEVELINFO_FILE_VERSION_3.0

name:                           The Puzzle Pits
author:                         Abe Pralle
levels:                         ${files.length}
first_level:                    1
`);
fs.writeFileSync(path.join(out, "graphics", "pits.png"), png(width, height, sheet));
// the ground drawn under everything, standing or moving (the pictures of
// what moves are see-through around it)
// and a playfield of whole squares (11 of 48 inside its 2-pixel border),
// in the middle of where the standard one is
let conf = "game.tile_size:                 48\ngame.use_masked_elements:       true\n" +
	"viewport.playfield.x:           14\nviewport.playfield.y:           14\nviewport.playfield.width:       532\nviewport.playfield.height:      532\n\n";
rows.forEach((r, j) => {
	conf += `${r.name}:${" ".repeat(Math.max(1, 32 - r.name.length))}pits.png\n${r.name}.xpos:${" ".repeat(Math.max(1, 27 - r.name.length))}0\n${r.name}.ypos:${" ".repeat(Math.max(1, 27 - r.name.length))}${j}\n${r.name}.frames:${" ".repeat(Math.max(1, 25 - r.name.length))}${r.frames.length}\n${r.name}.tile_size:${" ".repeat(Math.max(1, 22 - r.name.length))}${T}\n`;
	if (r.frames.length > 1) conf += `${r.name}.delay:${" ".repeat(Math.max(1, 26 - r.name.length))}4\n`;
	conf += "\n";
});
fs.writeFileSync(path.join(out, "graphics", "graphicsinfo.conf"), conf);
fs.writeFileSync(path.join(out, "README.txt"), `The Puzzle Pits for Rocks'n'Diamonds: Abe Pralle's 1995 levels as Sokoban,
made by tools/pit2rnd.mjs from https://github.com/LaPingvino/The-Puzzle-Pits.
What each level loses against the original:\n\n${report.join("\n")}\n`);
console.log(report.join("\n"));
console.log(`\n${out}: ${files.length} levels`);
