// script.mjs: a script's text (as tools/pit.mjs disassembles it) as a tree
// of statements and values, and back to the same text. The editor's blocks
// are this tree. What doesn't parse is kept as it is: a "code" statement
// (lines of text) or an "expr" value (a typed value).
//
// statements: { op: "IF", cmp, a, b, then: [...], else: [...] | null }
//             { op: "IFINPUT", text, then, else }   (text: as written, quoted)
//             { op: "FOR", r, from, to, body }
//             { op: "SWITCH", on: [...], off: [...] | null }
//             { op: "SET", how: LET ADD SUB MUL DIV, target, value }
//             { op: "FOUR", how: SWAPOVER SWAPUNDER GOTO, args: [4 values] }
//             { op: "CHANGEMAP", sets: [[x, y, tile] ...] }
//             { op: "TEXT", how: PRINT INPUT, text }
//             { op: "DO", how: REDRAW APPEAR STOP WIN TARGET POPUP REDGEMOFF ... }
//             { op: "code", lines: [...] }
// values:     { num } | { reg } | { lookup, args: [...] } | { raw } (?n) | { expr }
import { tokens, splitArgs, VALUE_NAMES } from "./pit.mjs";

export const SCRIPT_DO = ["REDRAW", "APPEAR", "STOP", "WIN", "TARGET", "POPUP", "REDGEMOFF", "GREENGEMOFF", "BLUEGEMOFF"];
export const SCRIPT_CMP = ["EQ", "NE", "LT", "GT", "LE", "GE"];

export function parseValue(t) {
	t = t.trim();
	if (/^-?\d+$/.test(t)) return { num: +t };
	if (/^\?\d+$/.test(t)) return { raw: t };
	if (VALUE_NAMES.registers.includes(t)) return { reg: t };
	const m = /^([A-Z]+)\((.*)\)$/.exec(t);
	if (m && VALUE_NAMES.lookups[m[1]] !== undefined) {
		const args = splitArgs(m[2]);
		if (args.length === VALUE_NAMES.lookups[m[1]]) return { lookup: m[1], args: args.map(parseValue) };
	}
	return { expr: t };
}

export function valueText(v) {
	if (v.num !== undefined) return String(v.num);
	if (v.raw) return v.raw;
	if (v.reg) return v.reg;
	if (v.lookup) return `${v.lookup}(${v.args.map(valueText).join(", ")})`;
	return v.expr ?? "0";
}

// one line as a statement (or null: not one by itself), with how it opens
// or closes a block
function parseLine(line) {
	const t = tokens(line);
	const vals = (k, from = 1) => (t.length === from + k ? t.slice(from).map(parseValue) : null);
	switch (t[0]) {
		case "IF":
			if (t[1] === "INPUT" && t.length === 3 && t[2].startsWith('"')) return { open: { op: "IFINPUT", text: t[2], then: [], else: null } };
			if (SCRIPT_CMP.includes(t[1]) && t.length === 4) { const [a, b] = vals(2, 2); return { open: { op: "IF", cmp: t[1], a, b, then: [], else: null } }; }
			return null;
		case "ELSE": case "ENDIF": case "NEXT": case "SWITCHON": case "SWITCHOFF":
			return t.length === 1 ? { mark: t[0] } : null;
		case "FOR": { const v = vals(3); return v ? { open: { op: "FOR", r: v[0], from: v[1], to: v[2], body: [] } } : null; }
		case "LET": case "ADD": case "SUB": case "MUL": case "DIV": { const v = vals(2); return v ? { stmt: { op: "SET", how: t[0], target: v[0], value: v[1] } } : null; }
		case "SWAPOVER": case "SWAPUNDER": case "GOTO": { const v = vals(4); return v ? { stmt: { op: "FOUR", how: t[0], args: v } } : null; }
		case "CHANGEMAP": {
			const sets = splitArgs(line.trim().slice(9)).map((s) => tokens(s));
			if (!sets.length || sets.some((s) => s.length !== 3)) return null;
			return { stmt: { op: "CHANGEMAP", sets: sets.map((s) => s.map(parseValue)) } };
		}
		case "PRINT": case "INPUT": return t.length === 2 && t[1].startsWith('"') ? { stmt: { op: "TEXT", how: t[0], text: t[1] } } : null;
		default: return SCRIPT_DO.includes(t[0]) && t.length === 1 ? { stmt: { op: "DO", how: t[0] } } : null;
	}
}

// the text as statements; a block that doesn't close, or a mark without
// its block, and lines that don't parse, stay text
export function parseScript(text) {
	const root = { list: [] };
	const stack = [{ node: root, list: root.list, kind: "root" }];
	const top = () => stack.at(-1);
	const code = (line) => {
		const list = top().list, last = list.at(-1);
		if (last?.op === "code") last.lines.push(line.trim());
		else list.push({ op: "code", lines: [line.trim()] });
	};
	for (const raw of text.split("\n")) {
		const line = raw.trim();
		if (!line) continue;
		const p = line.startsWith(";") ? null : parseLine(line);
		if (!p) { code(line); continue; }
		if (p.stmt) { top().list.push(p.stmt); continue; }
		if (p.open) {
			top().list.push(p.open);
			const list = p.open.op === "FOR" ? p.open.body : p.open.then;
			stack.push({ node: p.open, list, kind: p.open.op });
			continue;
		}
		const k = top().kind;
		switch (p.mark) {
			case "ELSE":
				if ((k === "IF" || k === "IFINPUT") && !top().node.else) { top().node.else = []; top().list = top().node.else; } else code(line);
				break;
			case "ENDIF":
				if (k === "IF" || k === "IFINPUT") stack.pop(); else code(line);
				break;
			case "NEXT":
				if (k === "FOR") stack.pop(); else code(line);
				break;
			case "SWITCHON":
				if (k === "root" && !root.list.some((s) => s.op === "SWITCH")) {
					const sw = { op: "SWITCH", on: [], off: null };
					top().list.push(sw);
					stack.push({ node: sw, list: sw.on, kind: "SWITCH" });
				} else code(line);
				break;
			case "SWITCHOFF":
				if (k === "SWITCH" && !top().node.off) { top().node.off = []; top().list = top().node.off; } else code(line);
				break;
		}
	}
	// blocks left open: as text again, so nothing changes
	while (stack.length > 1) {
		const open = stack.pop();
		if (open.kind === "SWITCH") continue; // the switch's part runs to the end
		const parent = top().list, i = parent.indexOf(open.node);
		parent.splice(i, 1, { op: "code", lines: scriptLines([open.node]).map((l) => l.trim()) });
		// (the unclosed block's own lines, without the ENDIF or NEXT it lacks)
		const c = parent[i];
		c.lines = c.lines.slice(0, -1);
	}
	return root.list;
}

// statements as text lines, indented two spaces a block as pit.mjs does
export function scriptLines(list, depth = 0) {
	const out = [], pad = "  ".repeat(depth);
	for (const s of list) {
		switch (s.op) {
			case "IF": case "IFINPUT":
				out.push(pad + (s.op === "IF" ? `IF ${s.cmp} ${valueText(s.a)} ${valueText(s.b)}` : `IF INPUT ${s.text}`));
				out.push(...scriptLines(s.then, depth + 1));
				if (s.else) { out.push(pad + "ELSE"); out.push(...scriptLines(s.else, depth + 1)); }
				out.push(pad + "ENDIF");
				break;
			case "FOR":
				out.push(pad + `FOR ${valueText(s.r)} ${valueText(s.from)} ${valueText(s.to)}`, ...scriptLines(s.body, depth + 1), pad + "NEXT");
				break;
			case "SWITCH":
				out.push(pad + "SWITCHON", ...scriptLines(s.on, depth + 1));
				if (s.off) out.push(pad + "SWITCHOFF", ...scriptLines(s.off, depth + 1));
				break;
			case "SET": out.push(pad + `${s.how} ${valueText(s.target)} ${valueText(s.value)}`); break;
			case "FOUR": out.push(pad + `${s.how} ${s.args.map(valueText).join(" ")}`); break;
			case "CHANGEMAP": out.push(pad + `CHANGEMAP ${s.sets.map((set) => set.map(valueText).join(" ")).join(", ")}`); break;
			case "TEXT": out.push(pad + `${s.how} ${s.text}`); break;
			case "DO": out.push(pad + s.how); break;
			case "code": out.push(...s.lines.map((l) => pad + l)); break;
		}
	}
	return out;
}
