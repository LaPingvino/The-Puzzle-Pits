// A level file from a text source:
//
//   node tools/mkpit.mjs LEVEL.pits [out.PIT]
//
// The source: `title:` and `password:` lines, then `map:` and the pit's
// rows (the first row is y = 1, the first column x = 1; the empty edge
// around the pit is added), then the scripts as tools/pitscripts.mjs
// writes them (`script <n> "<name>"`, `at x,y ...`, the statements
// indented). A `legend: C ground object` line sets what a map character
// means (numbers as the game has them), over the defaults below.
//
//   title: "The Anvil"
//   password: "Anvil"
//   map:
//   #######
//   #@.b..#
//   #######
//   script 0 "Master"
//   at
//   	SWITCHON
//   	SWITCHOFF
import fs from "node:fs";
import { writePit, SIZE } from "./pit.mjs";
import { mergeScripts } from "./pitscripts.mjs";

// map characters: [ground, object]
export const LEGEND = {
	" ": [0, 0], ".": [53, 0], "#": [53, 1], S: [53, 5], P: [53, 9], o: [53, 13], O: [53, 17], "@": [53, 100],
	b: [53, 29], c: [53, 37], k: [53, 45],
	1: [54, 0], 2: [55, 0], 3: [56, 0], ",": [57, 0], _: [58, 0], "=": [79, 0], "/": [71, 0],
	"^": [60, 0], ">": [61, 0], v: [62, 0], "<": [63, 0],
	R: [64, 0], G: [65, 0], B: [66, 0], Y: [67, 0], x: [68, 0], y: [69, 0], z: [70, 0], "!": [76, 0], "?": [77, 0], $: [78, 0],
	w: [81, 0], '"': [82, 0], h: [83, 0], "~": [84, 0], "%": [88, 0],
};

export function makePit(text) {
	const legend = { ...LEGEND };
	const grid = () => Array.from({ length: SIZE }, () => new Array(SIZE).fill(0));
	const level = { title: "", password: "", under: grid(), over: grid(), scripts: [], code: new Uint8Array(0), pitcode: null };
	const lines = text.split("\n");
	let i = 0, scripts = [];
	for (; i < lines.length; i++) {
		const l = lines[i], m = l.match(/^(title|password):\s*(".*")\s*$/);
		if (m) level[m[1]] = JSON.parse(m[2]);
		else if (/^legend:/.test(l)) {
			const [c, u, o] = l.replace(/^legend:\s?/, "").match(/^(.)\s+(\d+)\s+(\d+)/).slice(1);
			legend[c] = [+u, +o];
		} else if (/^map:\s*$/.test(l)) break;
		else if (l.trim() && !l.startsWith(";")) throw new Error(`line ${i + 1}: what is this? ${l}`);
	}
	let y = 1;
	for (i++; i < lines.length && !/^script\s/.test(lines[i]); i++, y++) {
		const row = lines[i];
		if (y >= SIZE - 1 && row.trim()) throw new Error(`line ${i + 1}: the pit is at most ${SIZE - 2} rows`);
		[...row].forEach((c, x) => {
			if (!(c in legend)) throw new Error(`line ${i + 1}: no legend for "${c}"`);
			if (x + 1 >= SIZE - 1) throw new Error(`line ${i + 1}: the pit is at most ${SIZE - 2} columns`);
			[level.under[x + 1][y], level.over[x + 1][y]] = legend[c];
		});
	}
	scripts = lines.slice(i).join("\n");
	const players = level.over.flat().filter((o) => o === 100).length;
	if (players !== 1) throw new Error(`the map needs exactly one player (@), it has ${players}`);
	return scripts.trim() ? mergeScripts(level, `title: ${JSON.stringify(level.title)}\npassword: ${JSON.stringify(level.password)}\n\n${scripts}`) : level;
}

if (process.argv[1] && new URL(import.meta.url).pathname === (await import("node:path")).resolve(process.argv[1])) {
	const [src, out = src.replace(/\.pits$/i, ".PIT")] = process.argv.slice(2);
	if (!src) {
		console.error("node tools/mkpit.mjs LEVEL.pits [out.PIT]");
		process.exit(2);
	}
	fs.writeFileSync(out, writePit(makePit(fs.readFileSync(src, "utf8"))));
	console.error(`${out}: ${makePit(fs.readFileSync(src, "utf8")).title}`);
}
