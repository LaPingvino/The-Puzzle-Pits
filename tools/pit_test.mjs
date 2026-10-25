// node tools/pit_test.mjs: every level reads and writes back as it was
import fs from "node:fs";
import { readPit, writePit } from "./pit.mjs";

const dir = new URL("../PITS95/LEVELS/", import.meta.url);
let bad = 0;
for (const name of fs.readdirSync(dir).filter((n) => /\.pit$/i.test(n)).sort()) {
	const bytes = new Uint8Array(fs.readFileSync(new URL(name, dir)));
	const level = readPit(bytes);
	const again = writePit(level);
	const same = again.length === bytes.length && again.every((b, i) => b === bytes[i]);
	if (!same) {
		bad++;
		const at = again.findIndex((b, i) => b !== bytes[i]);
		console.log(`${name}: differs at byte ${at} (${again.length} bytes, was ${bytes.length})`);
	}
	if (process.argv.includes("-v")) console.log(name, JSON.stringify(level.title), level.password, level.scripts.map((s) => s.name).join(" "));
}
console.log(bad ? `${bad} levels differ` : "all levels read and write back as they were");
if (bad) process.exit(1);

// every script disassembles and assembles back to the same bytes
import { disassemble, assemble } from "./pit.mjs";
let scripts = 0, badScripts = 0;
for (const name of fs.readdirSync(dir).filter((n) => /\.pit$/i.test(n)).sort()) {
	const level = readPit(new Uint8Array(fs.readFileSync(new URL(name, dir))));
	for (const s of level.scripts) {
		if (!s.size) continue;
		scripts++;
		const { lines, length } = disassemble(level.code, s.offset);
		const want = level.code.subarray(s.offset, s.offset + length);
		let got;
		try { got = assemble(lines); } catch (e) { got = null; console.log(`${name} ${s.name}: ${e.message}`); }
		if (!got || got.length !== want.length || got.some((b, i) => b !== want[i])) {
			badScripts++;
			if (got) console.log(`${name} ${s.name}: assembles differently at byte ${got.findIndex((b, i) => b !== want[i])} of ${want.length}`);
		}
		if (process.argv.includes("-s") && name === process.argv.at(-1)) console.log(`; ${s.name}\n${lines.join("\n")}\n`);
	}
}
console.log(badScripts ? `${badScripts} of ${scripts} scripts differ` : `all ${scripts} scripts assemble back as they were`);
if (badScripts) process.exit(1);
