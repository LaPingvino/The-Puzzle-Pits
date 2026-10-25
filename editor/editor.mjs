// The Puzzle Pits level editor: paints a pit's ground and objects, edits
// its scripts as text, and saves .PIT files that this port and the
// original PITS.EXE both read. Its data (the tiles, Abe's levels) comes in
// PITS_DATA, put in by tools/build-editor.mjs.

import { readPit, writePit, disassemble, assemble, SIZE } from "../tools/pit.mjs";
import { readShapes, drawSquare, blit, TILE } from "../tools/tiles.mjs";

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
function load(lv, name) {
	level = lv;
	file = name;
	if (!level.pitcode) level.pitcode = emptyGrid(NO_SCRIPT);
	scripts = level.scripts.map((s) => ({ index: s.index, name: s.name, text: s.size ? disassemble(level.code, s.offset).lines.join("\n") : "", empty: !s.size, size: s.size, original: s.size ? level.code.subarray(s.offset, s.offset + disassemble(level.code, s.offset).length) : null }));
	current = scripts.length ? 0 : -1;
	undo.length = redo.length = 0;
	$("title").value = level.title;
	$("password").value = level.password;
	$("file").textContent = name;
	showScripts();
	draw();
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

palette("objects", "over", OBJECTS);
palette("ground", "under", GROUND);
$("levels").value = "LEVEL01.PIT";
load(readPit(bytesOf(PITS_DATA.levels["LEVEL01.PIT"])), "LEVEL01.PIT");
pick("under", 53);
