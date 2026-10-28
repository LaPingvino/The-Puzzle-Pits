// node tools/pitsfont.mjs [PITS95-folder] [graphics-folder]: The Puzzle
// Pits' font (PLASMA1.FNT) as Rocks'n'Diamonds artwork for a level set:
// the scripts' text in its engraved letters on its grey stone (envelopes),
// and in white with a shadow where it stands on the pit (the engine's
// typing room and pointer). Writes PNGs and graphicsinfo.conf.
//
// PLASMA1.FNT: the characters from the space (32) on, each a byte of width,
// one of height (8), then width × height bytes (not 0: ink), as GFX.C
// reads it; a width of 0 ends it. Letters are proportional; R'n'D's fonts
// are a grid, so each goes in the middle of a cell.
import fs from "node:fs";
import zlib from "node:zlib";
import path from "node:path";

const root = new URL("../", import.meta.url).pathname;
const [data = path.join(root, "PITS95"), out = "graphics"] = process.argv.slice(2);

const ZOOM = 2; // a font pixel, as the pit's: 2 by 2
const CELL_W = 16, CELL_H = 18; // 30 columns of 16 fit R'n'D's playfield (560)
const PER_LINE = 16;

// the letters
const font = fs.readFileSync(path.join(data, "PLASMA1.FNT"));
const letters = [];
for (let p = 0; p < font.length && font[p] !== 0; ) {
	const w = font[p], h = font[p + 1];
	letters.push({ w, h, ink: font.subarray(p + 2, p + 2 + w * h) });
	p += 2 + w * h;
}

// the palette's greys (GFX.C SetPalette 1: colours 0, 1, 3, 5, 6)
const grey = (v) => [v * 4 + (v >> 4), v * 4 + (v >> 4), v * 4 + (v >> 4), 255];
const BLACK = grey(0), WHITE = grey(63), LIGHT = grey(50), STONE = grey(37), DARK = grey(25);

function sheet(style) {
	const lines = Math.ceil(letters.length / PER_LINE);
	const W = PER_LINE * CELL_W, H = lines * CELL_H;
	const img = new Uint8Array(W * H * 4);
	const dot = (x, y, c) => {
		if (x < 0 || y < 0 || x >= W || y >= H) return;
		img.set(c, (y * W + x) * 4);
	};
	letters.forEach((l, n) => {
		const cx = (n % PER_LINE) * CELL_W, cy = Math.floor(n / PER_LINE) * CELL_H;
		const x0 = cx + Math.floor((CELL_W - l.w * ZOOM) / 2), y0 = cy + Math.floor((CELL_H - l.h * ZOOM) / 2);
		// a letter, drawn at an offset in a colour (kept inside its cell)
		const draw = (dx, dy, c) => {
			for (let y = 0; y < l.h; y++) for (let x = 0; x < l.w; x++) {
				if (!l.ink[y * l.w + x]) continue;
				for (let j = 0; j < ZOOM; j++) for (let i = 0; i < ZOOM; i++) {
					const px = x0 + x * ZOOM + i + dx, py = y0 + y * ZOOM + j + dy;
					if (px >= cx && px < cx + CELL_W && py >= cy && py < cy + CELL_H) dot(px, py, c);
				}
			}
		};
		if (style === "stone") { // as Pits' StoneText: light below left, dark above right, then black
			draw(-1, 0, LIGHT); draw(0, 1, LIGHT); draw(1, 0, DARK); draw(0, -1, DARK); draw(0, 0, BLACK);
		} else { // as its ShadowText: black down and right, then white
			draw(ZOOM, ZOOM, BLACK); draw(0, 0, WHITE);
		}
	});
	return { W, H, img };
}

// the stone the text stands on: Pits' text boxes, grey with a light edge
// above and left and a dark one below and right (cut by R'n'D into a
// border of cells and an inside)
function stone() {
	const W = CELL_W * 3, H = CELL_H * 3, img = new Uint8Array(W * H * 4);
	for (let y = 0; y < H; y++) for (let x = 0; x < W; x++) {
		const lit = x < 2 || y < 2, shaded = x >= W - 2 || y >= H - 2;
		img.set(shaded ? DARK : lit ? LIGHT : STONE, (y * W + x) * 4);
	}
	return { W, H, img };
}

function png({ W, H, img }) {
	const crcTable = Array.from({ length: 256 }, (_, n) => { let c = n; for (let k = 0; k < 8; k++) c = c & 1 ? 0xedb88320 ^ (c >>> 1) : c >>> 1; return c >>> 0; });
	const crc = (b) => { let c = 0xffffffff; for (const x of b) c = crcTable[(c ^ x) & 0xff] ^ (c >>> 8); return (c ^ 0xffffffff) >>> 0; };
	const chunk = (type, body) => {
		const len = Buffer.alloc(4); len.writeUInt32BE(body.length);
		const tb = Buffer.concat([Buffer.from(type), body]);
		const c = Buffer.alloc(4); c.writeUInt32BE(crc(tb));
		return Buffer.concat([len, tb, c]);
	};
	const head = Buffer.alloc(13); head.writeUInt32BE(W, 0); head.writeUInt32BE(H, 4); head[8] = 8; head[9] = 6;
	const raw = Buffer.alloc((W * 4 + 1) * H);
	for (let y = 0; y < H; y++) Buffer.from(img.buffer, y * W * 4, W * 4).copy(raw, y * (W * 4 + 1) + 1);
	return Buffer.concat([Buffer.from([137, 80, 78, 71, 13, 10, 26, 10]), chunk("IHDR", head), chunk("IDAT", zlib.deflateSync(raw)), chunk("IEND", Buffer.alloc(0))]);
}

fs.mkdirSync(out, { recursive: true });
fs.writeFileSync(path.join(out, "pits_font_stone.png"), png(sheet("stone")));
fs.writeFileSync(path.join(out, "pits_font_shadow.png"), png(sheet("shadow")));
fs.writeFileSync(path.join(out, "pits_stone.png"), png(stone()));
const font_conf = (token, file) => [
	`${token}:`.padEnd(40) + file,
	...[["x", 0], ["y", 0], ["width", CELL_W], ["height", CELL_H], ["frames", letters.length], ["frames_per_line", PER_LINE]].map(([k, v]) => `${token}.${k}:`.padEnd(40) + v),
].join("\n");
fs.writeFileSync(path.join(out, "graphicsinfo.conf"), `# The Puzzle Pits' own letters (made by tools/pitsfont.mjs from PLASMA1.FNT)

# the scripts' text: engraved, on the game's grey stone
${font_conf("font.envelope_1", "pits_font_stone.png")}

background.envelope_1:                  pits_stone.png
background.envelope_1.x:                0
background.envelope_1.y:                0
background.envelope_1.width:            ${CELL_W * 3}
background.envelope_1.height:           ${CELL_H * 3}

# on the pit (the typing room, the pointer): white with a shadow
${font_conf("font.input_1", "pits_font_shadow.png")}
`);
console.log(`${letters.length} letters (${String.fromCharCode(32)} to ${String.fromCharCode(31 + letters.length)}) -> ${out}`);
