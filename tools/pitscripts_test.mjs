// node tools/pitscripts_test.mjs [levels-folder]: every level's scripts as
// text and back give the same level file; merged into a pit without
// scripts, the same scripts on the same squares
import fs from "node:fs";
import path from "node:path";
import { readPit, writePit, disassemble } from "./pit.mjs";
import { scriptsText, mergeScripts } from "./pitscripts.mjs";

const dir = process.argv[2] ?? new URL("../PITS95/LEVELS", import.meta.url).pathname;
const texts = (lv) => lv.scripts.map((s) => [s.index, s.name, s.size ? disassemble(lv.code, s.offset).lines : []]);
let bad = 0;
const files = fs.readdirSync(dir).filter((f) => /\.pit$/i.test(f)).sort();
for (const f of files) {
	const bytes = new Uint8Array(fs.readFileSync(path.join(dir, f)));
	const level = readPit(bytes), text = scriptsText(level, f);
	const back = writePit(mergeScripts(level, text));
	if (back.length !== bytes.length || back.some((b, i) => b !== bytes[i])) { bad++; console.log(`${f}: not the same file`); }
	const bare = readPit(writePit(mergeScripts({ ...level, scripts: [], code: new Uint8Array(0), pitcode: null }, text)));
	if (JSON.stringify(texts(bare)) !== JSON.stringify(texts(level)) || JSON.stringify(bare.pitcode) !== JSON.stringify(level.pitcode)) { bad++; console.log(`${f}: not the same scripts in a bare pit`); }
}
console.log(bad ? `${bad} wrong` : `all ${files.length} levels' scripts as text and back as they were`);
process.exit(bad ? 1 : 0);
