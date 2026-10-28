// node tools/rndset.mjs [PITS95-folder] [level-set-folder] [table.inc]: the
// artwork and template for a level set of R'n'D's own Puzzle Pits engine
// (filetype PITS), beside its LEVELnn.PIT files:
//
// - graphics/: the game's letters (tools/pitsfont.mjs), and pictures of
//   the custom elements R'n'D's editor shows a pit in, seen from above
//   (tools/rnd.mjs: a square's ground and what stands on it)
// - template.level: those elements, with their names
// - table.inc (if named): which element is which ground and object, for
//   R'n'D's files.c, which turns a pit into elements and back
import fs from "node:fs";
import zlib from "node:zlib";
import path from "node:path";
import { readShapes } from "./tiles.mjs";
import { rndArtwork, rndTemplate, rndPitsTable, RND_CE_NAMES, rndDescription } from "./rnd.mjs";
import { pitsFontArtwork } from "./pitsfont.mjs";

const root = new URL("../", import.meta.url).pathname;
const [data = path.join(root, "PITS95"), set = "The_Puzzle_Pits", table] = process.argv.slice(2);

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
	for (let y = 0; y < h; y++) Buffer.from(rgba.buffer, rgba.byteOffset + y * w * 4, w * 4).copy(raw, y * (w * 4 + 1) + 1);
	return Buffer.concat([Buffer.from([137, 80, 78, 71, 13, 10, 26, 10]), chunk("IHDR", head), chunk("IDAT", zlib.deflateSync(raw)), chunk("IEND", Buffer.alloc(0))]);
}

const gfx = path.join(set, "graphics");
fs.mkdirSync(gfx, { recursive: true });

// the letters
const font = pitsFontArtwork(data);
for (const [name, bytes] of Object.entries(font.files)) fs.writeFileSync(path.join(gfx, name), bytes);

// the elements' pictures (only theirs: the engine draws the pit itself)
const shapes = readShapes(...["TILES1.DAT", "TILES2.DAT", "TILES3.DAT"].map((f) => new Uint8Array(fs.readFileSync(path.join(data, f)))));
const art = rndArtwork(shapes, { customOnly: true });
fs.writeFileSync(path.join(gfx, "pits_elements.png"), png(art.width, art.height, art.rgba));
fs.writeFileSync(path.join(gfx, "graphicsinfo.conf"), font.conf +
	"\n# the squares of a pit, as R'n'D's editor shows them: custom elements, seen from above\n" +
	art.conf.replaceAll("pits.png", "pits_elements.png"));

// the elements, named, as the level set's template
fs.writeFileSync(path.join(set, "template.level"), rndTemplate());

// the editor's palette: the squares of a pit, grounds first, then what
// stands on them (R'n'D shows this list for a Puzzle Pits level)
{
	const rows = rndPitsTable();
	const order = (r) => (r[2] === 0 ? 0 : r[2] === 100 ? 1 : r[2] < 21 ? 2 : 3);
	const sorted = [...rows].sort((a, b) => order(a) - order(b));
	fs.writeFileSync(path.join(set, "editorsetup.conf"), "# The Puzzle Pits: R'n'D's editor shows these elements for its levels\n\n" +
		sorted.map(([e]) => `custom_${e - 359}`).join("\n") + "\n");
}

// the table for files.c
if (table) {
	const rows = rndPitsTable();
	fs.writeFileSync(table, `// The Puzzle Pits: a pit's squares as R'n'D's custom elements (the level
// set's template.level and artwork) and back: { element, ground, object }.
// Made by tools/rndset.mjs (https://github.com/LaPingvino/The-Puzzle-Pits)

${rows.map(([e, u, o]) => `  { ${String(e).padStart(3)}, ${String(u).padStart(2)}, ${String(o).padStart(3)} },	// ${rndDescription(RND_CE_NAMES[e - 360])}`).join("\n")}
`);
}
console.log(`${set}: graphics, template.level (${RND_CE_NAMES.length} elements)${table ? `; ${table}` : ""}`);
