// blocks.mjs: a level's scripts as blocks (Blockly, loaded when first
// shown), the same scripts as the text (tools/script.mjs is the tree both
// are). Text is a block too: a "code" block holds lines as typed, and a
// "typed value" block a value; either turns into blocks where it can, and
// any block turns into text, so blocks and typing mix, inside each other.

import { parseScript, scriptLines, parseValue, valueText, SCRIPT_DO, SCRIPT_CMP } from "../tools/script.mjs";
import { VALUE_NAMES } from "../tools/pit.mjs";

const BLOCKLY_URL = "https://cdn.jsdelivr.net/npm/blockly@10.4.3/blockly.min.js";

let blocklyLoading = null;
function loadBlockly() {
	if (window.Blockly) return Promise.resolve();
	blocklyLoading ??= new Promise((ok, fail) => {
		const s = document.createElement("script");
		s.src = BLOCKLY_URL;
		s.onload = ok;
		s.onerror = () => { blocklyLoading = null; fail(new Error("The blocks need Blockly, from the internet: this page couldn't load it")); };
		document.head.append(s);
	});
	return blocklyLoading;
}

// ---- the blocks

const C = { if: 210, loop: 120, set: 330, map: 20, text: 160, do: 290, code: 0, value: 230, lookup: 260 };
const statement = { previousStatement: null, nextStatement: null, extensions: ["pits_to_text"] };
const valueBlock = { output: "Value", extensions: ["pits_value_to_text"] };
const valueIn = (name) => ({ type: "input_value", name, check: "Value" });
const opts = (list) => list.map((x) => [x, x]);
const CMP_TEXT = { EQ: "=", NE: "≠", LT: "<", GT: ">", LE: "≤", GE: "≥" };

function defineBlocks(tiles) {
	const B = window.Blockly;
	B.defineBlocksWithJsonArray([
		{ type: "pits_switch", message0: "when switched on %1", args0: [{ type: "input_statement", name: "ON" }], ...statement, colour: C.if,
			tooltip: "What runs when the script is switched on (a plate pressed, a switch flipped on, a sign walked into; Intro and Master)" },
		{ type: "pits_switchonoff", message0: "when switched on %1 when switched off %2", args0: [{ type: "input_statement", name: "ON" }, { type: "input_statement", name: "OFF" }], ...statement, colour: C.if,
			tooltip: "What runs when switched on, and when switched off (a plate left, a switch flipped off)" },
		{ type: "pits_if", message0: "if %1 %2 %3 %4", args0: [valueIn("A"), { type: "field_dropdown", name: "CMP", options: SCRIPT_CMP.map((c) => [CMP_TEXT[c], c]) }, valueIn("B"), { type: "input_statement", name: "THEN" }], inputsInline: true, ...statement, colour: C.if },
		{ type: "pits_ifelse", message0: "if %1 %2 %3 %4 else %5", args0: [valueIn("A"), { type: "field_dropdown", name: "CMP", options: SCRIPT_CMP.map((c) => [CMP_TEXT[c], c]) }, valueIn("B"), { type: "input_statement", name: "THEN" }, { type: "input_statement", name: "ELSE" }], inputsInline: true, ...statement, colour: C.if },
		{ type: "pits_ifinput", message0: "if the answer is %1 %2", args0: [{ type: "field_input", name: "TEXT", text: "yes" }, { type: "input_statement", name: "THEN" }], ...statement, colour: C.if,
			tooltip: "If what the player typed (INPUT) starts with this, not minding capitals" },
		{ type: "pits_ifinputelse", message0: "if the answer is %1 %2 else %3", args0: [{ type: "field_input", name: "TEXT", text: "yes" }, { type: "input_statement", name: "THEN" }, { type: "input_statement", name: "ELSE" }], ...statement, colour: C.if },
		{ type: "pits_for", message0: "for %1 from %2 to %3 %4", args0: [valueIn("R"), valueIn("FROM"), valueIn("TO"), { type: "input_statement", name: "DO" }], inputsInline: true, ...statement, colour: C.loop,
			tooltip: "Counts the register from one value to the other (up or down), running what is inside each time" },
		{ type: "pits_set", message0: "%1 %2 %3", args0: [{ type: "field_dropdown", name: "HOW", options: [["set", "LET"], ["add to", "ADD"], ["subtract from", "SUB"], ["multiply", "MUL"], ["divide", "DIV"]] }, valueIn("TARGET"), valueIn("VALUE")], inputsInline: true, ...statement, colour: C.set,
			tooltip: "set R0 5, add to R0 1 (R0 = R0 + 1) ...; the first value can be a register, OVER, UNDER, WALKABLE or LASTPUSHED" },
		{ type: "pits_four", message0: "%1 %2 , %3 and %4 , %5", args0: [{ type: "field_dropdown", name: "HOW", options: [["swap what stands at", "SWAPOVER"], ["swap the ground at", "SWAPUNDER"], ["run the script of", "GOTO"]] }, valueIn("A"), valueIn("B"), valueIn("C"), valueIn("D")], inputsInline: true, ...statement, colour: C.map,
			tooltip: "Swap two squares (x1, y1 and x2, y2); or run the script of square x1, y1 as if at x2, y2" },
		{ type: "pits_changemap", message0: "change the map %1", args0: [{ type: "input_statement", name: "SETS", check: "Square" }], ...statement, colour: C.map },
		{ type: "pits_square", message0: "square %1 , %2 becomes %3", args0: [valueIn("X"), valueIn("Y"), valueIn("TILE")], inputsInline: true, previousStatement: "Square", nextStatement: "Square", colour: C.map,
			tooltip: "Select this, then click a square of the pit to fill in its x and y. A tile: an object (0 to 52, 100) or ground (53 to 100); -1 clears the ground, -2 what stands there, -3 both" },
		{ type: "pits_text", message0: "%1 %2", args0: [{ type: "field_dropdown", name: "HOW", options: [["show", "PRINT"], ["ask", "INPUT"]] }, { type: "field_multilinetext", name: "TEXT", text: "Hello!" }], ...statement, colour: C.text,
			tooltip: "Show text; or show it and let the player type an answer (see: if the answer is)" },
		{ type: "pits_do", message0: "%1", args0: [{ type: "field_dropdown", name: "HOW", options: [["win the level", "WIN"], ["stop the script", "STOP"], ["redraw", "REDRAW"], ["redraw, with magic", "APPEAR"], ["let the player point at a square", "TARGET"], ["put the plate up", "POPUP"], ["put away the red gem", "REDGEMOFF"], ["put away the green gem", "GREENGEMOFF"], ["put away the blue gem", "BLUEGEMOFF"]] }], ...statement, colour: C.do },
		{ type: "pits_code", message0: "%1", args0: [{ type: "field_multilinetext", name: "CODE", text: "" }], previousStatement: null, nextStatement: null, extensions: ["pits_code_menu"], colour: C.code,
			tooltip: "Script text, as typed: right-click to turn it into blocks" },
		{ type: "pits_num", message0: "%1", args0: [{ type: "field_number", name: "NUM", value: 0, min: -100, max: 100, precision: 1 }], ...valueBlock, colour: C.value },
		{ type: "pits_reg", message0: "%1", args0: [{ type: "field_dropdown", name: "REG", options: opts(VALUE_NAMES.registers) }], ...valueBlock, colour: C.value },
		{ type: "pits_tile", message0: "%1", args0: [{ type: "field_dropdown", name: "TILE", options: tiles.map(([n, label]) => [`${label} (${n})`, String(n)]) }], ...valueBlock, colour: C.value,
			tooltip: "A tile, by its name (it is its number)" },
		{ type: "pits_lookup", message0: "%1 ( %2 , %3 )", args0: [{ type: "field_dropdown", name: "LOOKUP", options: [["what stands at", "OVER"], ["the ground at", "UNDER"], ["last pushed at", "LASTPUSHED"], ["DELTA: add", "DELTA"]] }, valueIn("A"), valueIn("B")], inputsInline: true, ...valueBlock, colour: C.lookup },
		{ type: "pits_walkable", message0: "walkable %1", args0: [valueIn("A")], inputsInline: true, ...valueBlock, colour: C.lookup, tooltip: "Whether the player can walk on a tile (1 or 0); can be set" },
		{ type: "pits_expr", message0: "%1", args0: [{ type: "field_input", name: "EXPR", text: "R0" }], output: "Value", extensions: ["pits_expr_menu"], colour: C.code,
			tooltip: "A value, as typed: right-click to turn it into blocks" },
	]);
}

// ---- the tree as blocks (Blockly's JSON state), and back

const field = (block, name) => block.getFieldValue(name);

function valueState(v) {
	if (v.num !== undefined) return { type: "pits_num", fields: { NUM: v.num } };
	if (v.reg) return { type: "pits_reg", fields: { REG: v.reg } };
	if (v.lookup === "WALKABLE") return { type: "pits_walkable", inputs: { A: { block: valueState(v.args[0]) } } };
	if (v.lookup) return { type: "pits_lookup", fields: { LOOKUP: v.lookup }, inputs: { A: { block: valueState(v.args[0]) }, B: { block: valueState(v.args[1]) } } };
	return { type: "pits_expr", fields: { EXPR: v.raw ?? v.expr } };
}

const quoted = (s) => JSON.parse(s); // the text between the quotes, \n as a new line

function stmtState(s) {
	const chain = (list) => (list && list.length ? { block: chainState(list) } : undefined);
	const vals = (names, list) => Object.fromEntries(names.map((n, i) => [n, { block: valueState(list[i]) }]));
	switch (s.op) {
		case "SWITCH": return s.off ? { type: "pits_switchonoff", inputs: { ON: chain(s.on), OFF: chain(s.off) } } : { type: "pits_switch", inputs: { ON: chain(s.on) } };
		case "IF": return { type: s.else ? "pits_ifelse" : "pits_if", fields: { CMP: s.cmp }, inputs: { ...vals(["A", "B"], [s.a, s.b]), THEN: chain(s.then), ...(s.else ? { ELSE: chain(s.else) } : {}) } };
		case "IFINPUT": return { type: s.else ? "pits_ifinputelse" : "pits_ifinput", fields: { TEXT: quoted(s.text) }, inputs: { THEN: chain(s.then), ...(s.else ? { ELSE: chain(s.else) } : {}) } };
		case "FOR": return { type: "pits_for", inputs: { ...vals(["R", "FROM", "TO"], [s.r, s.from, s.to]), DO: chain(s.body) } };
		case "SET": return { type: "pits_set", fields: { HOW: s.how }, inputs: vals(["TARGET", "VALUE"], [s.target, s.value]) };
		case "FOUR": return { type: "pits_four", fields: { HOW: s.how }, inputs: vals(["A", "B", "C", "D"], s.args) };
		case "CHANGEMAP": {
			let first = null, last = null;
			for (const set of s.sets) {
				const b = { type: "pits_square", inputs: vals(["X", "Y", "TILE"], set) };
				if (last) last.next = { block: b }; else first = b;
				last = b;
			}
			return { type: "pits_changemap", inputs: first ? { SETS: { block: first } } : {} };
		}
		case "TEXT": return { type: "pits_text", fields: { HOW: s.how, TEXT: quoted(s.text) } };
		case "DO": return { type: "pits_do", fields: { HOW: s.how } };
		default: return { type: "pits_code", fields: { CODE: s.lines.join("\n") } };
	}
}

// Blockly wants no inputs that are empty
function tidy(state) {
	if (state?.inputs) for (const k of Object.keys(state.inputs)) if (!state.inputs[k]?.block) delete state.inputs[k];
	return state;
}

function chainState(list) {
	let first = null, last = null;
	for (const s of list) {
		const b = tidy(stmtState(s));
		if (last) last.next = { block: b }; else first = b;
		last = b;
	}
	return first;
}

function blockValue(b) {
	if (!b) return { expr: "?" }; // an empty slot: the text says so (and won't assemble)
	switch (b.type) {
		case "pits_num": return { num: Number(field(b, "NUM")) };
		case "pits_reg": return { reg: field(b, "REG") };
		case "pits_tile": return { num: Number(field(b, "TILE")) };
		case "pits_walkable": return { lookup: "WALKABLE", args: [blockValue(b.getInputTargetBlock("A"))] };
		case "pits_lookup": return { lookup: field(b, "LOOKUP"), args: [blockValue(b.getInputTargetBlock("A")), blockValue(b.getInputTargetBlock("B"))] };
		default: return { expr: field(b, "EXPR") || "?" };
	}
}

function blockStmts(b) {
	const out = [];
	for (; b; b = b.getNextBlock()) out.push(blockStmt(b));
	return out;
}

function blockStmt(b) {
	const v = (name) => blockValue(b.getInputTargetBlock(name));
	const list = (name) => blockStmts(b.getInputTargetBlock(name));
	const str = (name) => JSON.stringify(field(b, name));
	switch (b.type) {
		case "pits_switch": return { op: "SWITCH", on: list("ON"), off: null };
		case "pits_switchonoff": return { op: "SWITCH", on: list("ON"), off: list("OFF") };
		case "pits_if": case "pits_ifelse": return { op: "IF", cmp: field(b, "CMP"), a: v("A"), b: v("B"), then: list("THEN"), else: b.type === "pits_ifelse" ? list("ELSE") : null };
		case "pits_ifinput": case "pits_ifinputelse": return { op: "IFINPUT", text: str("TEXT"), then: list("THEN"), else: b.type === "pits_ifinputelse" ? list("ELSE") : null };
		case "pits_for": return { op: "FOR", r: v("R"), from: v("FROM"), to: v("TO"), body: list("DO") };
		case "pits_set": return { op: "SET", how: field(b, "HOW"), target: v("TARGET"), value: v("VALUE") };
		case "pits_four": return { op: "FOUR", how: field(b, "HOW"), args: ["A", "B", "C", "D"].map(v) };
		case "pits_changemap": {
			const sets = [];
			for (let s = b.getInputTargetBlock("SETS"); s; s = s.getNextBlock()) sets.push(["X", "Y", "TILE"].map((n) => blockValue(s.getInputTargetBlock(n))));
			return { op: "CHANGEMAP", sets };
		}
		case "pits_text": return { op: "TEXT", how: field(b, "HOW"), text: str("TEXT") };
		case "pits_do": return { op: "DO", how: field(b, "HOW") };
		default: return { op: "code", lines: (field(b, "CODE") || "").split("\n").filter((l) => l.trim()) };
	}
}

// ---- turning text into blocks and blocks into text, in place

function replaceStatement(old, state) {
	const B = window.Blockly, ws = old.workspace;
	B.Events.setGroup(true);
	const prev = old.previousConnection?.targetConnection, next = old.nextConnection?.targetConnection;
	const xy = old.getRelativeToSurfaceXY();
	if (next) old.nextConnection.disconnect();
	old.dispose(false);
	const made = state ? B.serialization.blocks.append(state, ws) : null;
	let last = made;
	while (last?.getNextBlock()) last = last.getNextBlock();
	if (made) {
		if (prev) prev.connect(made.previousConnection); else made.moveBy(xy.x, xy.y);
		if (next && last.nextConnection) last.nextConnection.connect(next);
	} else if (prev && next) prev.connect(next);
	B.Events.setGroup(false);
}

function replaceValue(old, state) {
	const B = window.Blockly, ws = old.workspace;
	B.Events.setGroup(true);
	const target = old.outputConnection?.targetConnection;
	const xy = old.getRelativeToSurfaceXY();
	old.dispose(false);
	const made = B.serialization.blocks.append(state, ws);
	if (target) target.connect(made.outputConnection); else made.moveBy(xy.x, xy.y);
	B.Events.setGroup(false);
}

function registerMenus() {
	const B = window.Blockly;
	B.Extensions.registerMixin("pits_code_menu", {
		customContextMenu(options) {
			options.unshift({ text: "Turn into blocks", enabled: true, callback: () => replaceStatement(this, chainState(parseScript(this.getFieldValue("CODE") || ""))) });
		},
	});
	B.Extensions.registerMixin("pits_to_text", {
		customContextMenu(options) {
			options.unshift({ text: "Turn into text", enabled: true, callback: () => replaceStatement(this, { type: "pits_code", fields: { CODE: scriptLines([blockStmt(this)]).join("\n") } }) });
		},
	});
	B.Extensions.registerMixin("pits_expr_menu", {
		customContextMenu(options) {
			options.unshift({ text: "Turn into blocks", enabled: true, callback: () => replaceValue(this, valueState(parseValue(this.getFieldValue("EXPR") || "0"))) });
		},
	});
	B.Extensions.registerMixin("pits_value_to_text", {
		customContextMenu(options) {
			options.unshift({ text: "Turn into text", enabled: true, callback: () => replaceValue(this, { type: "pits_expr", fields: { EXPR: valueText(blockValue(this)) } }) });
		},
	});
}

// ---- the toolbox

function toolbox() {
	const b = (type, extra = {}) => ({ kind: "block", type, ...extra });
	const num = (n) => ({ block: { type: "pits_num", fields: { NUM: n } } });
	return {
		kind: "categoryToolbox",
		contents: [
			{ kind: "category", name: "When", colour: C.if, contents: [b("pits_switch"), b("pits_switchonoff"), b("pits_if", { inputs: { A: { block: { type: "pits_reg", fields: { REG: "R0" } } }, B: num(0) } }), b("pits_ifelse", { inputs: { A: { block: { type: "pits_reg", fields: { REG: "R0" } } }, B: num(0) } }), b("pits_ifinput"), b("pits_ifinputelse"), b("pits_for", { inputs: { R: { block: { type: "pits_reg", fields: { REG: "R0" } } }, FROM: num(1), TO: num(10) } })] },
			{ kind: "category", name: "Do", colour: C.do, contents: [b("pits_text"), b("pits_do"), b("pits_set", { inputs: { TARGET: { block: { type: "pits_reg", fields: { REG: "R0" } } }, VALUE: num(1) } })] },
			{ kind: "category", name: "Map", colour: C.map, contents: [b("pits_changemap", { inputs: { SETS: { block: { type: "pits_square", inputs: { X: num(1), Y: num(1), TILE: { block: { type: "pits_tile", fields: { TILE: "53" } } } } } } } }), b("pits_square", { inputs: { X: num(1), Y: num(1), TILE: { block: { type: "pits_tile", fields: { TILE: "53" } } } } }), b("pits_four", { inputs: { A: num(1), B: num(1), C: num(2), D: num(2) } })] },
			{ kind: "category", name: "Values", colour: C.value, contents: [b("pits_num"), b("pits_reg"), b("pits_tile"), b("pits_lookup", { inputs: { A: { block: { type: "pits_reg", fields: { REG: "XPOS" } } }, B: { block: { type: "pits_reg", fields: { REG: "YPOS" } } } } }), b("pits_walkable", { inputs: { A: { block: { type: "pits_tile", fields: { TILE: "1" } } } } })] },
			{ kind: "category", name: "Typed", colour: C.code, contents: [b("pits_code", { fields: { CODE: "PRINT \"Hello!\"" } }), b("pits_expr")] },
		],
	};
}

// ---- the panel: scriptBlocks(element, tiles, changed) gives { show(text),
// text(), pickSquare(x, y) }; changed(text) is called when the blocks change

export async function scriptBlocks(element, tiles, changed) {
	await loadBlockly();
	const B = window.Blockly;
	if (!B.Blocks.pits_switch) { registerMenus(); defineBlocks(tiles); }
	const ws = B.inject(element, { toolbox: toolbox(), trashcan: true, zoom: { controls: true, wheel: true, startScale: 0.85 }, move: { scrollbars: true, drag: true, wheel: false }, renderer: "thrasos",
		theme: B.Theme.defineTheme("pits", { base: B.Themes.Classic, componentStyles: { toolboxBackgroundColour: "#2a2e37", toolboxForegroundColour: "#e4e6eb", flyoutBackgroundColour: "#353a45", flyoutOpacity: 0.95 } }) });
	let loading = false;
	// the script: the stacks of blocks one after another, top to bottom
	const text = () => {
		const tops = ws.getTopBlocks(true).filter((b) => b.previousConnection || b.nextConnection || b.type === "pits_code");
		return scriptLines(tops.flatMap((b) => blockStmts(b))).join("\n");
	};
	ws.addChangeListener((e) => {
		if (loading || e.isUiEvent) return;
		changed(text());
	});
	return {
		show(scriptText) {
			loading = true;
			ws.clear();
			const first = chainState(parseScript(scriptText || ""));
			if (first) B.serialization.blocks.append({ ...first, x: 10, y: 10 }, ws);
			loading = false;
		},
		text,
		resize() { B.svgResize(ws); },
		// a square clicked in the pit: into the selected square block
		pickSquare(x, y) {
			const sel = B.getSelected?.() ?? B.common?.getSelected?.();
			if (!sel || sel.type !== "pits_square") return false;
			for (const [name, n] of [["X", x], ["Y", y]]) {
				const old = sel.getInputTargetBlock(name);
				if (old) old.dispose(false);
				const made = B.serialization.blocks.append({ type: "pits_num", fields: { NUM: n } }, ws);
				sel.getInput(name).connection.connect(made.outputConnection);
			}
			return true;
		},
	};
}
