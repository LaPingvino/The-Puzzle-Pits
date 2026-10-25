// The Puzzle Pits level editor: paints a pit's ground and objects, edits
// its scripts as text, and saves .PIT files that this port and the
// original PITS.EXE both read. Its data (the tiles, Abe's levels) comes in
// PITS_DATA, put in by tools/build-editor.mjs.

import { readPit, writePit, disassemble, assemble, SIZE } from "../tools/pit.mjs";
import { readShapes, drawSquare, blit, TILE } from "../tools/tiles.mjs";
import { rndConvert, rndXsb, rndLevelFile, rndPitsLevelFile, rndPitsGrid, rndLost, rndLevelinfo, rndArtwork, rndCe } from "../tools/rnd.mjs";

const $ = (id) => document.getElementById(id);
const bytesOf = (b64) => Uint8Array.from(atob(b64), (c) => c.charCodeAt(0));
const shapes = readShapes(...PITS_DATA.tiles.map(bytesOf));

// the tiles to paint with: objects stand on the ground (the over layer),
// each the first of its four pictures; the ground is the under layer
const OBJECTS = [[1, "Wall"], [5, "Wall with text"], [9, "Pillar"], [13, "Switch (off)"], [17, "Switch (on)"],
	[21, "Barrel of water"], [25, "Barrel with a hole"], [29, "Barrel"], [33, "Blue barrel"], [37, "Crate"],
	[41, "Red crate"], [45, "Chest"], [49, "Green chest"], [100, "The player"]];
const GROUND = [[53, "Floor"], [54, "Red target"], [55, "Green target"], [56, "Blue target"], [57, "Marked tile"],
	[58, "Plate"], [59, "Plate (down)"], [60, "Arrow up"], [61, "Arrow right"], [62, "Arrow down"], [63, "Arrow left"],
	[64, "Red gem"], [65, "Green gem"], [66, "Blue gem"], [67, "Yellow gem"], [68, "Red key"], [69, "Green key"],
	[70, "Blue key"], [71, "Glass"], [76, "Red lock"], [77, "Green lock"], [78, "Blue lock"], [79, "Crate of water"],
	[80, "Crate with a hole"], [81, "Wood"], [82, "Grass"], [83, "Hole"], [84, "Water"], [88, "Lava"], [0, "Nothing (void)"]];
const PLAYER = 100;
const NO_SCRIPT = 255;

// the level being edited: as readPit gives it, its scripts as text
let level, scripts, current = -1, file = "untitled.pit";
let rndRows = null; // the level for Rocks'n'Diamonds as corrected by hand, or null: converted as it is
let tool = { layer: "under", tile: 53 }; // or { layer: "over", tile } or { layer: "script" } or { layer: "erase" }
const undo = [], redo = [];

function emptyGrid(v) { return Array.from({ length: SIZE }, (_, x) => Array.from({ length: SIZE }, (_, y) => (x && y && x < SIZE - 1 && y < SIZE - 1 ? v : 0))); }

function newLevel() {
	const lv = { password: "", title: "A New Pit", under: emptyGrid(53), over: emptyGrid(0), pitcode: emptyGrid(NO_SCRIPT), scripts: [], code: new Uint8Array(0) };
	for (let i = 1; i < SIZE - 1; i++) for (const [x, y] of [[i, 1], [i, SIZE - 2], [1, i], [SIZE - 2, i]]) lv.over[x][y] = 1;
	lv.over[11][11] = PLAYER;
	return lv;
}

// load a level: its scripts disassembled to text
function load(lv, name, rnd = null) {
	level = lv;
	file = name;
	rndRows = rnd;
	if (!level.pitcode) level.pitcode = emptyGrid(NO_SCRIPT);
	scripts = level.scripts.map((s) => ({ index: s.index, name: s.name, text: s.size ? disassemble(level.code, s.offset).lines.join("\n") : "", empty: !s.size, size: s.size, original: s.size ? level.code.subarray(s.offset, s.offset + disassemble(level.code, s.offset).length) : null }));
	current = scripts.length ? 0 : -1;
	undo.length = redo.length = 0;
	$("title").value = level.title;
	$("password").value = level.password;
	$("file").textContent = name;
	showScripts();
	draw();
	showRnd(true);
	status(`${name}: ${level.title}`);
}

// the level as the game reads it: scripts assembled, in order
function build() {
	const parts = [], defs = [];
	let offset = 0;
	for (const s of scripts) {
		let bytes = new Uint8Array(0), size = 0;
		if (!s.empty || s.text.trim()) {
			bytes = assemble(s.text.split("\n"));
			// unchanged: the size Abe's compiler gave it (5 more in early levels)
			size = s.original && bytes.length === s.original.length && bytes.every((b, i) => b === s.original[i]) ? s.size : bytes.length;
		}
		defs.push({ index: s.index, name: s.name, size, offset: bytes.length ? offset : 0 });
		parts.push(bytes);
		offset += bytes.length;
	}
	const code = new Uint8Array(offset);
	let at = 0;
	for (const p of parts) { code.set(p, at); at += p.length; }
	return { ...level, title: $("title").value, password: $("password").value, scripts: defs, code, trailing: undefined, pitcode: scripts.length ? level.pitcode : null };
}

// ---- the pit

const canvas = $("pit"), ctx = canvas.getContext("2d");
canvas.width = canvas.height = SIZE * TILE;
let hover = null;

function draw() {
	const img = ctx.createImageData(SIZE * TILE, SIZE * TILE);
	for (let y = 0; y < SIZE; y++) for (let x = 0; x < SIZE; x++) drawSquare(img.data, SIZE * TILE, shapes, level, x, y, x * TILE, y * TILE);
	ctx.putImageData(img, 0, 0);
	if ($("grid").checked) {
		ctx.strokeStyle = "rgba(255,255,255,.12)";
		for (let i = 1; i < SIZE; i++) {
			ctx.beginPath(); ctx.moveTo(i * TILE + .5, 0); ctx.lineTo(i * TILE + .5, SIZE * TILE); ctx.stroke();
			ctx.beginPath(); ctx.moveTo(0, i * TILE + .5); ctx.lineTo(SIZE * TILE, i * TILE + .5); ctx.stroke();
		}
	}
	// the squares with a script, and which
	if ($("showscripts").checked) {
		ctx.font = "bold 10px sans-serif";
		ctx.textAlign = "center";
		for (let y = 1; y < SIZE - 1; y++) for (let x = 1; x < SIZE - 1; x++) {
			const i = level.pitcode[x][y];
			if (i === NO_SCRIPT) continue;
			const k = scripts.findIndex((s) => s.index === i);
			ctx.fillStyle = k === current ? "rgba(255,200,0,.45)" : "rgba(80,160,255,.35)";
			ctx.fillRect(x * TILE + 1, y * TILE + 1, TILE - 2, TILE - 2);
			ctx.fillStyle = "#fff";
			ctx.fillText(k >= 0 ? String(k + 1) : "?", x * TILE + TILE / 2, y * TILE + TILE / 2 + 4);
		}
	}
	if (hover) {
		ctx.strokeStyle = "#ff0";
		ctx.strokeRect(hover[0] * TILE + .5, hover[1] * TILE + .5, TILE - 1, TILE - 1);
	}
	if (rndRows === null) showRnd();
}

function squareAt(e) {
	const r = canvas.getBoundingClientRect();
	const x = Math.floor(((e.clientX - r.left) / r.width) * SIZE), y = Math.floor(((e.clientY - r.top) / r.height) * SIZE);
	return x >= 0 && y >= 0 && x < SIZE && y < SIZE ? [x, y] : null;
}

function snapshot() {
	return JSON.stringify({ under: level.under, over: level.over, pitcode: level.pitcode });
}
function restore(s) { Object.assign(level, JSON.parse(s)); draw(); }
function remember() { undo.push(snapshot()); if (undo.length > 200) undo.shift(); redo.length = 0; }

function paint([x, y]) {
	if (x < 1 || y < 1 || x > SIZE - 2 || y > SIZE - 2) { status("The pit's edge is always empty"); return; }
	if (tool.layer === "under") level.under[x][y] = tool.tile;
	else if (tool.layer === "over") {
		if (tool.tile === PLAYER) for (const col of level.over) for (let j = 0; j < SIZE; j++) if (col[j] === PLAYER) col[j] = 0; // only one
		level.over[x][y] = tool.tile;
	} else if (tool.layer === "script") {
		if (current < 0) { status("Make a script first (+ Script)"); return; }
		level.pitcode[x][y] = scripts[current].index;
		// the game runs a square's script only for these (PITS.C CheckStuff, CheckGuy)
		const o = level.over[x][y], u = level.under[x][y];
		const runs = u === 58 || u === 59 || (o >= 5 && o <= 8) || (o >= 13 && o <= 20);
		status(runs ? `${scripts[current].name} on ${x}, ${y}` : `${scripts[current].name} on ${x}, ${y}: it runs only on a plate, a wall with text or a switch, so put one there`);
	} else { // erase: the object, else the script, else the ground to floor
		if (level.over[x][y]) level.over[x][y] = 0;
		else if (level.pitcode[x][y] !== NO_SCRIPT) level.pitcode[x][y] = NO_SCRIPT;
		else level.under[x][y] = 53;
	}
	draw();
}

let painting = false;
canvas.addEventListener("contextmenu", (e) => e.preventDefault());
canvas.addEventListener("pointerdown", (e) => {
	const sq = squareAt(e);
	if (!sq) return;
	if (e.button === 2) { // pick up what is there
		const [x, y] = sq;
		const o = level.over[x][y];
		if (o) pick("over", o >= PLAYER ? PLAYER : o - ((o - 1) % 4));
		else pick("under", level.under[x][y]);
		if (level.pitcode[x][y] !== NO_SCRIPT) selectScript(scripts.findIndex((s) => s.index === level.pitcode[x][y]));
		return;
	}
	remember();
	painting = true;
	canvas.setPointerCapture(e.pointerId);
	paint(sq);
});
canvas.addEventListener("pointermove", (e) => {
	const sq = squareAt(e);
	hover = sq;
	if (sq) {
		const [x, y] = sq, i = level.pitcode[x][y];
		const s = scripts.find((s) => s.index === i);
		$("where").textContent = `${x}, ${y}` + (s ? ` · script ${s.name}` : "");
	}
	if (painting && sq) paint(sq); else draw();
});
canvas.addEventListener("pointerup", () => (painting = false));
canvas.addEventListener("pointerleave", () => { hover = null; draw(); });

// ---- the palettes

function preview(layer, tile) {
	const c = document.createElement("canvas");
	c.width = c.height = layer === "over" ? TILE * 2 : TILE;
	const g = c.getContext("2d"), img = g.createImageData(c.width, c.height);
	if (layer === "over") { // the four quarters, as on the pit
		blit(img.data, c.width, shapes[53], TILE, TILE, false);
		[[0, 0, 0], [1, TILE, 0], [2, 0, TILE], [3, TILE, TILE]].forEach(([part, px, py]) => blit(img.data, c.width, shapes[tile + part], px, py, true));
	} else blit(img.data, c.width, shapes[tile], 0, 0, false);
	g.putImageData(img, 0, 0);
	return c;
}

function palette(id, layer, tiles) {
	for (const [tile, name] of tiles) {
		const b = document.createElement("button");
		b.className = "tile";
		b.title = name;
		b.dataset.layer = layer;
		b.dataset.tile = tile;
		b.append(preview(layer, tile));
		b.addEventListener("click", () => pick(layer, tile));
		$(id).append(b);
	}
}

function pick(layer, tile) {
	tool = { layer, tile };
	for (const b of document.querySelectorAll(".tile, .toolbtn")) b.classList.toggle("on", b.dataset.layer === layer && (layer === "script" || layer === "erase" || +b.dataset.tile === tile));
	const all = layer === "over" ? OBJECTS : GROUND;
	status(layer === "script" ? "Click squares to run the chosen script there" : layer === "erase" ? "Click to clear an object, then a script, then the ground" : `Painting: ${(all.find(([t]) => t === tile) ?? [, tile])[1]}`);
}

// ---- the scripts

function showScripts() {
	const list = $("scripts");
	list.innerHTML = "";
	scripts.forEach((s, k) => {
		const o = document.createElement("option");
		o.value = k;
		o.textContent = `${k + 1}. ${s.name}${s.empty ? " (empty)" : ""}`;
		list.append(o);
	});
	list.value = current;
	$("code").value = current >= 0 ? scripts[current].text : "";
	$("code").disabled = current < 0;
	$("scriptname").value = current >= 0 ? scripts[current].name : "";
	$("scripterror").textContent = "";
}

function selectScript(k) {
	if (k < 0) return;
	keepText();
	current = k;
	showScripts();
	draw();
}

// the text in the box goes back into the script, checked
function keepText() {
	if (current < 0) return true;
	const s = scripts[current];
	s.text = $("code").value;
	s.name = $("scriptname").value.trim() || s.name;
	try {
		if (s.text.trim()) assemble(s.text.split("\n"));
		s.empty = !s.text.trim();
		$("scripterror").textContent = "";
		return true;
	} catch (e) {
		$("scripterror").textContent = `${s.name}: ${e.message}`;
		return false;
	}
}

$("scripts").addEventListener("change", (e) => selectScript(+e.target.value));
$("code").addEventListener("change", () => { keepText(); showScriptsKeepingText(); });
$("scriptname").addEventListener("change", () => { keepText(); showScriptsKeepingText(); });
function showScriptsKeepingText() { const err = $("scripterror").textContent; showScripts(); $("scripterror").textContent = err; }

$("addscript").addEventListener("click", () => {
	keepText();
	const used = new Set(scripts.map((s) => s.index));
	let index = 0;
	while (used.has(index)) index++;
	if (index >= 100) { status("A level has at most 100 scripts"); return; }
	scripts.push({ index, name: `Script${index}`, text: "SWITCHON\n  PRINT \"Hello!\"\nSWITCHOFF", empty: false });
	current = scripts.length - 1;
	showScripts();
	pick("script");
});
$("delscript").addEventListener("click", () => {
	if (current < 0) return;
	const s = scripts[current];
	if (!confirm(`Remove the script ${s.name}, and from the squares it is on?`)) return;
	for (const col of level.pitcode) for (let j = 0; j < SIZE; j++) if (col[j] === s.index) col[j] = NO_SCRIPT;
	scripts.splice(current, 1);
	current = Math.min(current, scripts.length - 1);
	showScripts();
	draw();
});

// ---- files

function status(s) { $("status").textContent = s; }

function save() {
	if (!keepText()) { status("Fix the script first"); return null; }
	try {
		return writePit(build());
	} catch (e) {
		status(e.message);
		return null;
	}
}

$("save").addEventListener("click", () => {
	const bytes = save();
	if (!bytes) return;
	const a = document.createElement("a");
	a.href = URL.createObjectURL(new Blob([bytes], { type: "application/octet-stream" }));
	a.download = /\.pit$/i.test(file) ? file : file + ".pit";
	a.click();
	status(`Saved ${a.download}: copy it into the game's LEVELS folder (as LEVELnn.PIT)`);
});

$("play").addEventListener("click", () => {
	const bytes = save();
	if (!bytes) return;
	try {
		localStorage.setItem("puzzle-pits-play", btoa(String.fromCharCode(...bytes)));
		window.open(PITS_DATA.game + "?play", "puzzle-pits-play");
	} catch (e) {
		status("Could not hand the level to the game: " + e.message);
	}
});

$("open").addEventListener("change", async (e) => {
	const f = e.target.files[0];
	if (!f) return;
	try { load(readPit(new Uint8Array(await f.arrayBuffer())), f.name); } catch (err) { status(`${f.name}: ${err.message}`); }
	e.target.value = "";
});

$("levels").addEventListener("change", (e) => {
	if (!keepPackLevel()) return;
	packAt = -1;
	showPack();
	const name = e.target.value;
	if (name === "new") load(newLevel(), "untitled.pit");
	else load(readPit(bytesOf(PITS_DATA.levels[name])), name);
});

for (const [name] of Object.entries(PITS_DATA.levels)) {
	const o = document.createElement("option");
	o.value = name;
	o.textContent = name.replace(/\.PIT$/i, "");
	$("levels").append(o);
}

$("grid").addEventListener("change", draw);
$("showscripts").addEventListener("change", draw);
$("scripttool").addEventListener("click", () => pick("script"));
$("erasetool").addEventListener("click", () => pick("erase"));
addEventListener("keydown", (e) => {
	if (e.target.tagName === "TEXTAREA" || e.target.tagName === "INPUT") return;
	if ((e.ctrlKey || e.metaKey) && e.key.toLowerCase() === "z") {
		e.preventDefault();
		const from = e.shiftKey ? redo : undo, to = e.shiftKey ? undo : redo;
		if (from.length) { to.push(snapshot()); restore(from.pop()); }
	}
});

// ---- Rocks'n'Diamonds: the level as Sokoban rows (tools/rnd.mjs), shown
// as R'n'D will draw it; the rows can be corrected by hand, and the level
// (or the pack) downloaded as an R'n'D level set

const rndArt = rndArtwork(shapes);
const rndSheet = document.createElement("canvas");
rndSheet.width = rndArt.width;
rndSheet.height = rndArt.height;
rndSheet.getContext("2d").putImageData(new ImageData(rndArt.rgba, rndArt.width, rndArt.height), 0, 0);

function showRnd(fresh) {
	const auto = rndConvert(level);
	const rows = rndRows ?? auto.rows;
	if (fresh || (rndRows === null && document.activeElement !== $("rndrows"))) $("rndrows").value = rows.join("\n");
	const lost = rndRows !== null ? auto.lost : rndLost(level);
	$("rndlost").textContent = (rndRows !== null ? "Corrected by hand: plays as plain Sokoban. " : "With Pits' rules. ") + (lost.length ? "Not in R'n'D: " + lost.join(", ") + "." : "Plays as in Pits.");
	const S = 24, view = $("rndview"), g = view.getContext("2d"), T = rndArt.tile;
	g.imageSmoothingEnabled = false;
	if (rndRows === null) { // as R'n'D will draw it, element by element
		const { grid } = rndPitsGrid(level), w = Math.max(...grid.map((r) => r.length));
		view.width = w * S;
		view.height = grid.length * S;
		grid.forEach((r, y) => r.forEach((e, x) => {
			if (e !== 62) g.drawImage(rndSheet, 0, rndArt.where[0], T, T, x * S, y * S, S, S); // the ground under it
			g.drawImage(rndSheet, 0, rndArt.where[e] ?? rndArt.where[62], T, T, x * S, y * S, S, S);
			if (e === rndCe("start")) g.drawImage(rndSheet, 0, rndArt.where["@"], T, T, x * S, y * S, S, S); // the player starts here
		}));
		return;
	}
	const w = Math.max(1, ...rows.map((r) => r.length));
	view.width = w * S;
	view.height = Math.max(1, rows.length) * S;
	rows.forEach((r, y) => [...r.padEnd(w, "_")].forEach((c, x) => {
		if (c !== "_") g.drawImage(rndSheet, 0, rndArt.where[" "], T, T, x * S, y * S, S, S); // the ground under it
		const at = rndArt.where[c];
		if (at === undefined) { g.fillStyle = "#f44"; g.fillRect(x * S + 6, y * S + 6, S - 12, S - 12); return; }
		g.drawImage(rndSheet, 0, at, T, T, x * S, y * S, S, S);
	}));
}
$("rndrows").addEventListener("input", (e) => { rndRows = e.target.value.replace(/\s+$/, "").split("\n"); showRnd(); });
$("rndreset").addEventListener("click", () => { rndRows = null; showRnd(true); });

$("rndsave").addEventListener("click", async () => {
	if (!keepPackLevel()) return;
	const entries = pack.length ? pack.map((p) => ({ level: readPit(p.bytes), rnd: p.rnd ?? null })) : [{ level: build(), rnd: rndRows }];
	const name = (pack.length && $("packname").value.trim()) || entries[0].level.title || "My Pits";
	const author = $("packauthor").value.trim();
	const folder = name.toLowerCase().replace(/[^a-z0-9]+/g, "_").replace(/^_|_$/g, "") || "pits";
	const enc = new TextEncoder(), files = [];
	let xsb = `; ${name}\n\n`;
	entries.forEach(({ level: lv, rnd }, k) => {
		const rows = rnd ?? rndConvert(lv).rows;
		files.push({ name: `${folder}/${String(k + 1).padStart(3, "0")}.level`, bytes: rnd ? rndLevelFile(lv.title, author, rows) : rndPitsLevelFile(lv.title, author, lv).bytes });
		xsb += `; ${k + 1}\n'${lv.title}'\n${rndXsb(rows)}\n\n`;
	});
	const png = new Uint8Array(await (await new Promise((ok) => rndSheet.toBlob(ok, "image/png"))).arrayBuffer());
	files.push({ name: `${folder}/levelinfo.conf`, bytes: enc.encode(rndLevelinfo(name, author, entries.length)) },
		{ name: `${folder}/levels.xsb`, bytes: enc.encode(xsb) },
		{ name: `${folder}/graphics/pits.png`, bytes: png },
		{ name: `${folder}/graphics/graphicsinfo.conf`, bytes: enc.encode(rndArt.conf) });
	const a = document.createElement("a");
	a.href = URL.createObjectURL(zip(files));
	a.download = folder + "-rocksndiamonds.zip";
	a.click();
	status(`Saved ${a.download}: unpack it into ~/.rocksndiamonds/levels`);
});

// ---- level packs: a folder of LEVEL01.PIT, LEVEL02.PIT ... and pack.txt,
// shared as a zip (the game finds them in its PACKS folder)

const pack = []; // { name, bytes }
let packAt = -1; // the pack's level being edited, -1 for none

function showPack() {
	const list = $("pack");
	list.innerHTML = "";
	pack.forEach((p, k) => {
		const o = document.createElement("option");
		o.value = k;
		let title = "";
		try { title = readPit(p.bytes).title; } catch {}
		o.textContent = `LEVEL${String(k + 1).padStart(2, "0")} · ${title}`;
		list.append(o);
	});
	list.value = packAt;
}

// keep the edits to the pack's level being edited
function keepPackLevel() {
	if (packAt < 0) return true;
	const bytes = save();
	if (!bytes) return false;
	pack[packAt].bytes = bytes;
	pack[packAt].rnd = rndRows;
	return true;
}

$("pack").addEventListener("change", (e) => {
	if (!keepPackLevel()) { $("pack").value = packAt; return; }
	packAt = +e.target.value;
	load(readPit(pack[packAt].bytes), `LEVEL${String(packAt + 1).padStart(2, "0")}.PIT`, pack[packAt].rnd);
	showPack();
});
$("packadd").addEventListener("click", () => {
	if (!keepPackLevel()) return;
	const bytes = save();
	if (!bytes) return;
	pack.push({ bytes, rnd: rndRows });
	packAt = pack.length - 1;
	file = `LEVEL${String(pack.length).padStart(2, "0")}.PIT`;
	$("file").textContent = file;
	showPack();
	status(`Added as ${file}`);
});
function move(d) {
	const k = packAt, j = k + d;
	if (k < 0 || j < 0 || j >= pack.length || !keepPackLevel()) return;
	[pack[k], pack[j]] = [pack[j], pack[k]];
	packAt = j;
	showPack();
}
$("packup").addEventListener("click", () => move(-1));
$("packdown").addEventListener("click", () => move(1));
$("packremove").addEventListener("click", () => {
	if (packAt < 0 || !confirm("Remove this level from the pack?")) return;
	pack.splice(packAt, 1);
	packAt = -1;
	showPack();
});

// a zip of the pack, stored (not compressed): a folder named after it
const CRC = Array.from({ length: 256 }, (_, n) => { let c = n; for (let k = 0; k < 8; k++) c = c & 1 ? 0xedb88320 ^ (c >>> 1) : c >>> 1; return c >>> 0; });
function crc32(b) { let c = 0xffffffff; for (const x of b) c = CRC[(c ^ x) & 0xff] ^ (c >>> 8); return (c ^ 0xffffffff) >>> 0; }
function zip(files) {
	const enc = new TextEncoder(), parts = [], dir = [];
	let off = 0;
	for (const { name, bytes } of files) {
		const n = enc.encode(name), crc = crc32(bytes);
		const head = new DataView(new ArrayBuffer(30));
		head.setUint32(0, 0x04034b50, true); head.setUint16(4, 20, true); head.setUint32(14, crc, true);
		head.setUint32(18, bytes.length, true); head.setUint32(22, bytes.length, true); head.setUint16(26, n.length, true);
		const cen = new DataView(new ArrayBuffer(46));
		cen.setUint32(0, 0x02014b50, true); cen.setUint16(4, 20, true); cen.setUint16(6, 20, true); cen.setUint32(16, crc, true);
		cen.setUint32(20, bytes.length, true); cen.setUint32(24, bytes.length, true); cen.setUint16(28, n.length, true); cen.setUint32(42, off, true);
		parts.push(new Uint8Array(head.buffer), n, bytes);
		dir.push(new Uint8Array(cen.buffer), n);
		off += 30 + n.length + bytes.length;
	}
	const size = dir.reduce((a, b) => a + b.length, 0);
	const end = new DataView(new ArrayBuffer(22));
	end.setUint32(0, 0x06054b50, true); end.setUint16(8, files.length, true); end.setUint16(10, files.length, true);
	end.setUint32(12, size, true); end.setUint32(16, off, true);
	return new Blob([...parts, ...dir, new Uint8Array(end.buffer)], { type: "application/zip" });
}
// the files in a zip: stored or deflated (by the browser)
async function unzip(buf) {
	const v = new DataView(buf), out = [];
	let e = buf.byteLength - 22;
	while (e >= 0 && v.getUint32(e, true) !== 0x06054b50) e--;
	if (e < 0) throw new Error("not a zip");
	let p = v.getUint32(e + 16, true);
	for (let k = v.getUint16(e + 10, true); k > 0; k--) {
		const method = v.getUint16(p + 10, true), csize = v.getUint32(p + 20, true);
		const nlen = v.getUint16(p + 28, true), xlen = v.getUint16(p + 30, true), clen = v.getUint16(p + 32, true), local = v.getUint32(p + 42, true);
		const name = new TextDecoder().decode(new Uint8Array(buf, p + 46, nlen));
		const start = local + 30 + v.getUint16(local + 26, true) + v.getUint16(local + 28, true);
		let bytes = new Uint8Array(buf, start, csize);
		if (method === 8) bytes = new Uint8Array(await new Response(new Blob([bytes]).stream().pipeThrough(new DecompressionStream("deflate-raw"))).arrayBuffer());
		else if (method !== 0) throw new Error(`${name}: compressed in a way this cannot read`);
		out.push({ name, bytes });
		p += 46 + nlen + xlen + clen;
	}
	return out;
}

$("packsave").addEventListener("click", () => {
	if (!keepPackLevel()) return;
	if (!pack.length) { status("Add a level to the pack first"); return; }
	const name = $("packname").value.trim() || "My Pits";
	const folder = name.replace(/[^A-Za-z0-9_-]+/g, "_");
	const files = pack.map((p, k) => ({ name: `${folder}/LEVEL${String(k + 1).padStart(2, "0")}.PIT`, bytes: p.bytes }));
	// hand corrections for Rocks'n'Diamonds, kept with the pack (the game skips them)
	pack.forEach((p, k) => { if (p.rnd) files.push({ name: `${folder}/rnd/LEVEL${String(k + 1).padStart(2, "0")}.xsb`, bytes: new TextEncoder().encode(p.rnd.join("\n") + "\n") }); });
	files.push({ name: `${folder}/pack.txt`, bytes: new TextEncoder().encode(`name=${name}\nauthor=${$("packauthor").value.trim()}\n`) });
	const a = document.createElement("a");
	a.href = URL.createObjectURL(zip(files));
	a.download = folder + ".zip";
	a.click();
	status(`Saved ${a.download}: unpack it into the game's PACKS folder`);
});

$("packopen").addEventListener("change", async (e) => {
	const chosen = [...e.target.files];
	e.target.value = "";
	try {
		let files = [];
		for (const f of chosen) {
			if (/\.zip$/i.test(f.name)) files.push(...(await unzip(await f.arrayBuffer())));
			else files.push({ name: f.name, bytes: new Uint8Array(await f.arrayBuffer()) });
		}
		const info = files.find((f) => /(^|\/)pack\.txt$/i.test(f.name));
		const levels = files.filter((f) => /level\d+\.pit$/i.test(f.name)).sort((a, b) => +a.name.match(/(\d+)\.pit$/i)[1] - +b.name.match(/(\d+)\.pit$/i)[1]);
		if (!levels.length) throw new Error("no LEVELnn.PIT files in it");
		for (const l of levels) readPit(l.bytes); // all of them levels
		pack.length = 0;
		const fix = (l) => files.find((f) => f.name.toLowerCase().endsWith("rnd/" + l.name.split("/").pop().replace(/\.pit$/i, ".xsb").toLowerCase()));
		pack.push(...levels.map((l) => { const f = fix(l); return { bytes: l.bytes, rnd: f ? new TextDecoder().decode(f.bytes).replace(/\s+$/, "").split("\n") : null }; }));
		const txt = info ? new TextDecoder().decode(info.bytes) : "";
		$("packname").value = (/^name=(.*)$/m.exec(txt) || [, ""])[1].trim();
		$("packauthor").value = (/^author=(.*)$/m.exec(txt) || [, ""])[1].trim();
		packAt = 0;
		load(readPit(pack[0].bytes), "LEVEL01.PIT", pack[0].rnd);
		showPack();
		status(`Opened a pack of ${pack.length} levels`);
	} catch (err) {
		status(`Could not open the pack: ${err.message}`);
	}
});

palette("objects", "over", OBJECTS);
palette("ground", "under", GROUND);
$("levels").value = "LEVEL01.PIT";
load(readPit(bytesOf(PITS_DATA.levels["LEVEL01.PIT"])), "LEVEL01.PIT");
pick("under", 53);
