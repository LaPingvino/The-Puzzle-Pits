// pit.mjs: The Puzzle Pits' level files (.PIT), read and written as the
// game does (PITS.C GameLoadLevel, FILE.C ReadString and ReadRun), for the
// level editor and for tools. Works in a browser and in Node.

const MAGIC = 0x0895;
const STRING_KEY = "Mindy"; // FILE.C cryptkey
const CODE_KEY = "Amanda"; // PITS.C GameLoadLevel key
export const SIZE = 23; // a pit is 23 by 23 squares, its edge always empty

// xor applies a key to bytes, as FILE.C's Decrypt (the same both ways).
function xor(bytes, key) {
	const out = Uint8Array.from(bytes);
	for (let i = 0; i < out.length; i++) out[i] ^= key.charCodeAt(i % key.length);
	return out;
}

class Reader {
	constructor(bytes) { this.b = bytes; this.i = 0; }
	byte() {
		if (this.i >= this.b.length) throw new Error("the file ends too soon");
		return this.b[this.i++];
	}
	word() { const hi = this.byte(); return (hi << 8) | this.byte(); } // high byte first
	bytes(n) {
		if (this.i + n > this.b.length) throw new Error("the file ends too soon");
		const out = this.b.subarray(this.i, this.i + n);
		this.i += n;
		return out;
	}
	// a string: its length XOR 22, then its bytes XOR STRING_KEY
	string() {
		const n = this.word() ^ 22;
		return String.fromCharCode(...xor(this.bytes(n), STRING_KEY));
	}
	// a 23 by 23 map, run-length coded, skipping its empty edge
	// (ReadRun); grid[x][y], as the game's under[x][y]
	run() {
		const flat = new Array(SIZE * SIZE).fill(0);
		let i = 24;
		while (i < 505) {
			let n = this.byte();
			const tile = this.byte();
			while (n > 0 && i < flat.length) {
				flat[i++] = tile;
				if ((i + 1) % SIZE === 0) i += 2; // the edge between rows
				n--;
			}
		}
		return Array.from({ length: SIZE }, (_, x) => flat.slice(x * SIZE, (x + 1) * SIZE));
	}
}

class Writer {
	constructor() { this.b = []; }
	byte(v) { this.b.push(v & 0xff); }
	word(v) { this.byte(v >> 8); this.byte(v); }
	bytes(a) { for (const v of a) this.byte(v); }
	string(s) {
		this.word(s.length ^ 22);
		this.bytes(xor(Uint8Array.from(s, (c) => c.charCodeAt(0)), STRING_KEY));
	}
	run(grid) {
		const flat = grid.flat();
		const cells = []; // the squares ReadRun fills, in its order
		for (let i = 24; i < 505; ) {
			cells.push(flat[i++]);
			if ((i + 1) % SIZE === 0) i += 2;
		}
		for (let i = 0; i < cells.length; ) {
			let n = 1;
			while (i + n < cells.length && cells[i + n] === cells[i] && n < 255) n++;
			this.byte(n);
			this.byte(cells[i]);
			i += n;
		}
	}
	result() { return Uint8Array.from(this.b); }
}

// read a level file's bytes: { password, title, under, over, pitcode,
// scripts: [{ index, name, offset, size }], code }, code as the game
// runs it (decrypted)
export function readPit(bytes) {
	const r = new Reader(bytes);
	if (r.word() !== MAGIC) throw new Error("not a Puzzle Pits level");
	const level = { password: r.string(), title: r.string(), under: r.run(), over: r.run() };
	let n = r.word();
	level.pitcode = null;
	level.scripts = [];
	level.code = new Uint8Array(0);
	if (n) {
		level.pitcode = r.run();
		while (n--) {
			const index = r.word();
			const name = r.string();
			const size = r.word();
			const offset = r.word();
			level.scripts.push({ index, name, size, offset });
		}
		level.code = xor(r.bytes(r.word()), CODE_KEY);
	}
	if (r.i !== bytes.length) level.trailing = bytes.subarray(r.i); // kept as found
	return level;
}

// write a level as the game reads it
export function writePit(level) {
	const w = new Writer();
	w.word(MAGIC);
	w.string(level.password);
	w.string(level.title);
	w.run(level.under);
	w.run(level.over);
	w.word(level.scripts.length);
	if (level.scripts.length) {
		w.run(level.pitcode);
		for (const s of level.scripts) {
			w.word(s.index);
			w.string(s.name);
			w.word(s.size);
			w.word(s.offset);
		}
		w.word(level.code.length);
		w.bytes(xor(level.code, CODE_KEY));
	}
	if (level.trailing) w.bytes(level.trailing);
	return w.result();
}

// ---- Scripts --------------------------------------------------------
// A level's scripts are bytecode that PITS.C RunPitCode interprets. Here
// they become text, one statement a line, and back to the same bytes.

const OPS = ["END", "ENDIF", "NEXT", "ELSE", "SWITCHOFF", "SWITCHON", "IFEQ", "IFLT", "IFGT", "IFLE", "IFGE", "IFNE", "IFINPUT",
	"FOR", "REDRAW", "APPEAR", "STOP", "WIN", "TARGET", "REDGEMOFF", "GREENGEMOFF", "BLUEGEMOFF", "POPUP", "LET", "ADD", "SUB",
	"MUL", "DIV", "SWAPOVER", "SWAPUNDER", "GOTO", "CHANGEMAP", "PRINT", "INPUT"];
const OP = Object.fromEntries(OPS.map((n, i) => [n, i]));
// registers 201..235 (TILEDEFS.H); 201..210 are the scripts' own, R0..R9
const REGS = { 211: "XGUY", 212: "YGUY", 213: "LASTX", 214: "LASTY", 215: "FACING", 216: "REDGEMS", 217: "GREENGEMS",
	218: "BLUEGEMS", 219: "YELLOWGEMS", 220: "REDKEYS", 221: "GREENKEYS", 222: "BLUEKEYS", 223: "INPUTVALUE", 224: "XPOS",
	225: "YPOS", 226: "MOVES", 227: "HECTOMOVES", 233: "REDGEMON", 234: "GREENGEMON", 235: "BLUEGEMON" };
for (let i = 0; i < 10; i++) REGS[201 + i] = "R" + i;
const REG = Object.fromEntries(Object.entries(REGS).map(([k, v]) => [v, +k]));
// those that look something up: their arguments follow
const LOOKUPS = { 228: ["WALKABLE", 1], 229: ["DELTA", 2], 230: ["UNDER", 2], 231: ["OVER", 2], 232: ["LASTPUSHED", 2] };
const LOOKUP = Object.fromEntries(Object.entries(LOOKUPS).map(([k, [n, a]]) => [n, [+k, a]]));

// a value at c[p]: a number (0..100, 101..200 for -100..-1), a register
// or a lookup; returns [text, next position]
function value(c, p) {
	const b = c[p++];
	if (b <= 100) return [String(b), p];
	if (b <= 200) return [String(b - 201), p];
	if (LOOKUPS[b]) {
		const [name, n] = LOOKUPS[b];
		const args = [];
		for (let i = 0; i < n; i++) { let a; [a, p] = value(c, p); args.push(a); }
		return [`${name}(${args.join(", ")})`, p];
	}
	return [REGS[b] ?? `?${b}`, p];
}

const word = (c, p) => (c[p] << 8) | c[p + 1];
const text = (bytes) => JSON.stringify(String.fromCharCode(...bytes));

// disassemble the script at offset: its lines, indented by block, and its
// length (to its END)
export function disassemble(c, offset) {
	const lines = [];
	let p = offset, depth = 0;
	const values = (n) => { const out = []; for (let i = 0; i < n; i++) { let v; [v, p] = value(c, p); out.push(v); } return out; };
	for (;;) {
		if (p >= c.length) throw new Error("a script runs past the code");
		const op = c[p++];
		const name = OPS[op];
		if (name === undefined) throw new Error(`unknown command ${op} at ${p - 1}`);
		if (op === OP.END) break;
		let line;
		if (op >= OP.IFEQ && op <= OP.IFNE) { line = `IF ${name.slice(2)} ${values(2).join(" ")}`; p += 2; }
		else if (op === OP.IFINPUT) { const n = word(c, p); line = `IF INPUT ${text(c.subarray(p + 4, p + 4 + n))}`; p += 4 + n; }
		else if (op === OP.FOR) { line = `FOR ${values(3).join(" ")}`; p += 2; }
		else if (op === OP.ELSE || op === OP.SWITCHON) p += 2;
		else if (op >= OP.LET && op <= OP.DIV) line = `${name} ${values(2).join(" ")}`;
		else if (op >= OP.SWAPOVER && op <= OP.GOTO) line = `${name} ${values(4).join(" ")}`;
		else if (op === OP.CHANGEMAP) {
			const end = p + 2 + word(c, p);
			p += 2;
			const sets = [];
			while (p < end) sets.push(values(3).join(" "));
			line = `CHANGEMAP ${sets.join(", ")}`;
		} else if (op === OP.PRINT || op === OP.INPUT) { const n = word(c, p); line = `${name} ${text(c.subarray(p + 2, p + 2 + n))}`; p += 2 + n; }
		if (op === OP.ENDIF || op === OP.NEXT || op === OP.ELSE || op === OP.SWITCHOFF) depth = Math.max(0, depth - 1);
		lines.push("  ".repeat(depth) + (line ?? name));
		if (op === OP.ELSE || op === OP.SWITCHOFF || op === OP.SWITCHON || op === OP.FOR || (op >= OP.IFEQ && op <= OP.IFINPUT)) depth++;
	}
	return { lines, length: p - offset };
}

// ---- assembling text back into bytecode

// split a line into words, strings and parenthesised lookups kept whole
function tokens(line) {
	const out = [];
	const re = /\s*("(?:[^"\\]|\\.)*"|[A-Z]+\([^()]*(?:\([^()]*\)[^()]*)*\)|,|[^\s,]+)/gy;
	let m;
	while ((m = re.exec(line)) && m[1] !== undefined) out.push(m[1]);
	return out;
}

function encodeValue(t, out) {
	if (/^-?\d+$/.test(t)) {
		const n = +t;
		if (n < -100 || n > 100) throw new Error(`${t} is out of range (-100 to 100)`);
		out.push(n < 0 ? n + 201 : n);
		return;
	}
	const m = /^([A-Z]+)\((.*)\)$/.exec(t);
	if (m) {
		const [code, n] = LOOKUP[m[1]] ?? [];
		if (code === undefined) throw new Error(`unknown lookup ${m[1]}`);
		const args = splitArgs(m[2]);
		if (args.length !== n) throw new Error(`${m[1]} takes ${n} values`);
		out.push(code);
		for (const a of args) encodeValue(a, out);
		return;
	}
	if (/^\?\d+$/.test(t)) { out.push(+t.slice(1)); return; }
	if (REG[t] === undefined) throw new Error(`unknown value ${t}`);
	out.push(REG[t]);
}

function splitArgs(s) {
	const args = [];
	let depth = 0, cur = "";
	for (const ch of s) {
		if (ch === "," && depth === 0) { args.push(cur.trim()); cur = ""; continue; }
		if (ch === "(") depth++;
		if (ch === ")") depth--;
		cur += ch;
	}
	if (cur.trim()) args.push(cur.trim());
	return args;
}

const stringBytes = (t) => Array.from(JSON.parse(t), (ch) => ch.charCodeAt(0) & 0xff);

// assemble a script's lines into bytecode; jumps are worked out from the
// blocks, as Abe's compiler made them: IF to its ELSE or ENDIF, ELSE to its
// ENDIF, FOR past its NEXT, SWITCHON to its SWITCHOFF
export function assemble(lines) {
	const out = [];
	const open = []; // blocks: { kind, at (where the jump's two bytes are) }
	const patch = (at, target) => { const off = target - at; out[at] = off >> 8; out[at + 1] = off & 0xff; };
	lines.forEach((raw, n) => {
		const line = raw.trim();
		if (!line || line.startsWith(";")) return;
		try {
			const t = tokens(line);
			const vals = (k, from = 1) => {
				if (t.length !== from + k) throw new Error(`${t[0]} takes ${k} values`);
				for (let i = from; i < from + k; i++) encodeValue(t[i], out);
			};
			const jumpHere = () => { const at = out.length; out.push(0, 0); return at; };
			switch (t[0]) {
				case "IF": {
					if (t[1] === "INPUT") {
						const s = stringBytes(t[2]);
						out.push(OP.IFINPUT, s.length >> 8, s.length & 0xff);
						open.push({ kind: "IF", at: jumpHere() });
						out.push(...s);
					} else {
						const op = OP["IF" + t[1]];
						if (op === undefined) throw new Error(`unknown comparison ${t[1]}`);
						out.push(op);
						vals(2, 2);
						open.push({ kind: "IF", at: jumpHere() });
					}
					break;
				}
				case "ELSE": {
					const b = open.pop();
					if (b?.kind !== "IF") throw new Error("ELSE without IF");
					out.push(OP.ELSE);
					patch(b.at, out.length - 1);
					open.push({ kind: "ELSE", at: jumpHere() });
					break;
				}
				case "ENDIF": {
					const b = open.pop();
					if (b?.kind !== "IF" && b?.kind !== "ELSE") throw new Error("ENDIF without IF");
					out.push(OP.ENDIF);
					patch(b.at, out.length - 1);
					break;
				}
				case "FOR":
					out.push(OP.FOR);
					vals(3);
					open.push({ kind: "FOR", at: jumpHere() });
					break;
				case "NEXT": {
					const b = open.pop();
					if (b?.kind !== "FOR") throw new Error("NEXT without FOR");
					out.push(OP.NEXT);
					patch(b.at, out.length);
					break;
				}
				case "SWITCHON":
					out.push(OP.SWITCHON);
					open.push({ kind: "SWITCHON", at: jumpHere() });
					break;
				case "SWITCHOFF": {
					if (open.at(-1)?.kind === "SWITCHON") patch(open.pop().at, out.length);
					out.push(OP.SWITCHOFF);
					open.push({ kind: "SWITCHOFF" }); // indents what follows, ends with the script
					break;
				}
				case "CHANGEMAP": {
					out.push(OP.CHANGEMAP, 0, 0);
					const start = out.length;
					const sets = splitArgs(t.slice(1).filter((x) => x !== ",").length ? line.slice(9) : "");
					for (const set of sets) {
						const v = tokens(set);
						if (v.length !== 3) throw new Error("CHANGEMAP takes x y tile, separated by commas");
						for (const x of v) encodeValue(x, out);
					}
					const n = out.length - start;
					out[start - 2] = n >> 8;
					out[start - 1] = n & 0xff;
					break;
				}
				case "PRINT":
				case "INPUT": {
					const s = stringBytes(t[1]);
					out.push(OP[t[0]], s.length >> 8, s.length & 0xff, ...s);
					break;
				}
				default: {
					const op = OP[t[0]];
					if (op === undefined || op === OP.END) throw new Error(`unknown command ${t[0]}`);
					out.push(op);
					if (op >= OP.LET && op <= OP.DIV) vals(2);
					else if (op >= OP.SWAPOVER && op <= OP.GOTO) vals(4);
					else if (t.length > 1) throw new Error(`${t[0]} takes nothing`);
				}
			}
		} catch (e) {
			throw new Error(`line ${n + 1}: ${e.message}`);
		}
	});
	const left = open.filter((b) => b.kind !== "SWITCHOFF");
	if (left.length) throw new Error(`${left.at(-1).kind} is not closed`);
	out.push(OP.END);
	return Uint8Array.from(out);
}
