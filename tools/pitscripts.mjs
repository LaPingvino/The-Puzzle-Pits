// A level's scripts as a text file, and back into a level file:
//
//   node tools/pitscripts.mjs LEVEL01.PIT > LEVEL01.pits
//   node tools/pitscripts.mjs LEVEL01.PIT LEVEL01.pits [out.PIT]
//
// The first writes the scripts out; the second puts them into the level
// (its title and password too, if the text has them), keeping its pit:
// so a pit drawn elsewhere (in Rocks'n'Diamonds' level editor, which saves
// .PIT files without touching their scripts, or anywhere else) gets the
// scripts it should have. Written to out.PIT, else over the level.
//
// The text: a script starts with a line `script <number> "<name>"`, then
// `at x,y ...` with the squares it's on (the pit's own numbers, 0..22),
// then its statements, one a line, as tools/pit.mjs shows them; a line
// starting with ; is a comment.
//
//   title: "Starting Out"
//   password: "BEGIN"
//
//   script 0 "Intro"
//   at 5,4
//   	POPUP "You stand at the edge of your very first pit."
//   	END
import fs from "node:fs";
import path from "node:path";
import { readPit, writePit, disassemble, assemble, SIZE } from "./pit.mjs";

const NO_SCRIPT = 255;

// a level's scripts as text
export function scriptsText(level, name = "") {
	const out = [`; The Puzzle Pits: ${name ? name + "'s " : ""}scripts (tools/pitscripts.mjs)`, "",
		`title: ${JSON.stringify(level.title)}`, `password: ${JSON.stringify(level.password)}`];
	for (const s of level.scripts) {
		const at = [];
		if (level.pitcode) for (let y = 0; y < SIZE; y++) for (let x = 0; x < SIZE; x++) if (level.pitcode[x][y] === s.index) at.push(`${x},${y}`);
		out.push("", `script ${s.index} ${JSON.stringify(s.name)}`, `at ${at.join(" ")}`.trimEnd());
		if (s.size) for (const l of disassemble(level.code, s.offset).lines) out.push("\t" + l);
	}
	return out.join("\n") + "\n";
}

// the scripts of a text: { title, password, scripts: [{ index, name, at, lines }] }
export function parseScriptsText(text) {
	const got = { scripts: [] };
	let s = null;
	text.split("\n").forEach((raw, n) => {
		const line = raw.trim();
		const fail = (why) => { throw new Error(`line ${n + 1}: ${why}`); };
		if (!line || line.startsWith(";")) return;
		let m;
		if (!/^\s/.test(raw) && (m = line.match(/^(title|password):\s*(".*")$/))) got[m[1]] = JSON.parse(m[2]);
		else if (!/^\s/.test(raw) && (m = line.match(/^script\s+(\d+)\s+(".*")$/))) got.scripts.push(s = { index: +m[1], name: JSON.parse(m[2]), at: [], lines: [] });
		else if (!/^\s/.test(raw) && /^at(\s|$)/.test(line)) {
			if (!s) fail("`at` before a script");
			for (const sq of line.slice(2).trim().split(/\s+/).filter(Boolean)) {
				const [x, y] = sq.split(",").map(Number);
				if (!(x >= 0 && x < SIZE && y >= 0 && y < SIZE)) fail(`not a square: ${sq}`);
				s.at.push([x, y]);
			}
		} else if (s && /^\s/.test(raw)) s.lines.push(line);
		else fail(`what is this? ${line}`);
	});
	return got;
}

// a level with the text's scripts (and title and password)
export function mergeScripts(level, text) {
	const got = parseScriptsText(text);
	const old = new Map(level.scripts.filter((s) => s.size).map((s) => [s.index, s]));
	const pitcode = Array.from({ length: SIZE }, () => new Array(SIZE).fill(NO_SCRIPT));
	const parts = [], defs = [];
	for (const s of got.scripts) {
		let bytes;
		try { bytes = s.lines.length ? assemble(s.lines) : new Uint8Array(0); } catch (e) { throw new Error(`script ${s.index} ${JSON.stringify(s.name)}: ${e.message}`); }
		// unchanged: the size Abe's compiler gave it (5 more in early levels)
		const was = old.get(s.index);
		const same = was && (() => { const o = level.code.subarray(was.offset, was.offset + disassemble(level.code, was.offset).length); return o.length === bytes.length && o.every((b, i) => b === bytes[i]); })();
		defs.push({ index: s.index, name: s.name, size: bytes.length ? (same ? was.size : bytes.length) : 0, offset: 0, was: was?.offset });
		for (const [x, y] of s.at) pitcode[x][y] = s.index;
		parts.push(bytes);
	}
	// the code: a script the level had where it was, new ones after, by number
	const key = (d) => (d.was === undefined ? 1e6 + d.index : d.was);
	const order = defs.map((d, i) => i).sort((i, j) => key(defs[i]) - key(defs[j]));
	const code = new Uint8Array(parts.reduce((n, p) => n + p.length, 0));
	let at = 0;
	for (const i of order) {
		if (parts[i].length) defs[i].offset = at;
		code.set(parts[i], at);
		at += parts[i].length;
	}
	return { ...level, title: got.title ?? level.title, password: got.password ?? level.password, scripts: defs.map(({ was, ...d }) => d), code, pitcode: defs.length ? pitcode : null };
}

if (process.argv[1] && new URL(import.meta.url).pathname === path.resolve(process.argv[1])) {
	const [pit, txt, out = pit] = process.argv.slice(2);
	if (!pit) {
		console.error("node tools/pitscripts.mjs LEVEL.PIT [scripts.pits [out.PIT]]");
		process.exit(2);
	}
	const level = readPit(new Uint8Array(fs.readFileSync(pit)));
	if (!txt) process.stdout.write(scriptsText(level, path.basename(pit)));
	else {
		fs.writeFileSync(out, writePit(mergeScripts(level, fs.readFileSync(txt, "utf8"))));
		console.error(`${out}: ${level.title}, scripts from ${txt}`);
	}
}
