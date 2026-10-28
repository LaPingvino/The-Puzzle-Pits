// node tools/pit2rnd.mjs [LEVELS-folder] [out-folder]: The Puzzle Pits'
// levels as a Rocks'n'Diamonds level set, with the game's own pictures
// (tools/rnd.mjs does the work; the level editor exports the same way).
import fs from "node:fs";
import zlib from "node:zlib";
import path from "node:path";
import { readPit } from "./pit.mjs";
import { readShapes } from "./tiles.mjs";
import { rndConvert, rndXsb, rndPitsLevelFile, rndLost, rndLevelinfo, rndArtwork } from "./rnd.mjs";

const root = new URL("../", import.meta.url).pathname;
const [levelsDir = path.join(root, "PITS95/LEVELS"), out = "the_puzzle_pits"] = process.argv.slice(2);
const data = path.join(root, "PITS95");

function png(w, h, rgba) {
	const crcTable = Array.from({ length: 256 }, (_, n) => { let c = n; for (let k = 0; k < 8; k++) c = c & 1 ? 0xedb88320 ^ (c >>> 1) : c >>> 1; return c >>> 0; });
	const crc = (b) => { let c = 0xffffffff; for (const x of b) c = crcTable[(c ^ x) & 0xff] ^ (c >>> 8); return (c ^ 0xffffffff) >>> 0; };
	const chunk = (type, body) => {
		const len = Buffer.alloc(4); len.writeUInt32BE(body.length);
		const tb = Buffer.concat([Buffer.from(type), body]);
		const c = Buffer.alloc(4); c.writeUInt32BE(crc(tb));
		return Buffer.concat([len, tb, c]);
	};
	const head = Buffer.alloc(13); head.writeUInt32BE(w, 0); head.writeUInt32BE(h, 4); head[8] = 8; head[9] = 6;
	const raw = Buffer.alloc((w * 4 + 1) * h);
	for (let y = 0; y < h; y++) Buffer.from(rgba.buffer, y * w * 4, w * 4).copy(raw, y * (w * 4 + 1) + 1);
	return Buffer.concat([Buffer.from([137, 80, 78, 71, 13, 10, 26, 10]), chunk("IHDR", head), chunk("IDAT", zlib.deflateSync(raw)), chunk("IEND", Buffer.alloc(0))]);
}

const files = fs.readdirSync(levelsDir).filter((n) => /level\d+\.pit$/i.test(n)).sort();
let xsb = "; The Puzzle Pits, by Abe Pralle (Plasma Works), 1995: its levels as Sokoban\n\n";
const report = [];
fs.mkdirSync(path.join(out, "graphics"), { recursive: true });
files.forEach((name, i) => {
	const level = readPit(new Uint8Array(fs.readFileSync(path.join(levelsDir, name))));
	const { rows } = rndConvert(level), lost = rndLost(level);
	xsb += `; ${i + 1}\n'${level.title}'\n${rndXsb(rows)}\n\n`;
	fs.writeFileSync(path.join(out, `${String(i + 1).padStart(3, "0")}.level`), rndPitsLevelFile(level.title, "Abe Pralle", level).bytes);
	report.push(`${String(i + 1).padStart(2)} ${level.title.padEnd(26)} ${lost.length ? lost.join(", ") : "as in Pits"}`);
});
fs.writeFileSync(path.join(out, "levels.xsb"), xsb); // the same levels as Sokoban text, for other Sokoban games
fs.writeFileSync(path.join(out, "levelinfo.conf"), rndLevelinfo("The Puzzle Pits (custom elements)", "Abe Pralle", files.length));
const art = rndArtwork(readShapes(...["TILES1.DAT", "TILES2.DAT", "TILES3.DAT"].map((f) => new Uint8Array(fs.readFileSync(path.join(data, f))))));
fs.writeFileSync(path.join(out, "graphics", "pits.png"), png(art.width, art.height, art.rgba));
fs.writeFileSync(path.join(out, "graphics", "graphicsinfo.conf"), art.conf);
fs.writeFileSync(path.join(out, "README.txt"), `The Puzzle Pits for Rocks'n'Diamonds: Abe Pralle's 1995 levels, with Pits'
own rules as custom elements (coloured targets, arrows, glass, water, lava,
holes), made by tools/pit2rnd.mjs from https://github.com/LaPingvino/The-Puzzle-Pits.
levels.xsb has them as plain Sokoban, for other Sokoban games.
What each level loses against the original:\n\n${report.join("\n")}\n`);
console.log(report.join("\n"));
console.log(`\n${out}: ${files.length} levels`);
