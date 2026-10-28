// node tools/script_test.mjs: every script of Abe's levels as a tree
// (tools/script.mjs, what the editor's blocks are) and back, the same text
// and bytes, with nothing left as typed text
import fs from "node:fs";
import { readPit, disassemble, assemble } from "./pit.mjs";
import { parseScript, scriptLines } from "./script.mjs";

const dir = new URL("../PITS95/LEVELS/", import.meta.url);
let n = 0, failed = 0;
for (const f of fs.readdirSync(dir).filter((x) => /\.pit$/i.test(x)).sort()) {
	const level = readPit(new Uint8Array(fs.readFileSync(new URL(f, dir))));
	for (const s of level.scripts) {
		if (!s.size) continue;
		n++;
		const text = disassemble(level.code, s.offset).lines.join("\n");
		const tree = parseScript(text);
		const back = scriptLines(tree).join("\n");
		const a = assemble(text.split("\n")), b = assemble(back.split("\n"));
		const typed = JSON.stringify(tree).includes('"op":"code"');
		if (back !== text || a.length !== b.length || a.some((x, i) => x !== b[i]) || typed) {
			failed++;
			console.log(`${f} ${s.name}: ${typed ? "left as text" : "not the same"}`);
		}
	}
}
console.log(failed ? `${failed} of ${n} scripts differ` : `all ${n} scripts are blocks and back as they were`);
process.exit(failed ? 1 : 0);
