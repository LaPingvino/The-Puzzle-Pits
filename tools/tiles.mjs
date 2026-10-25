// tiles.mjs: The Puzzle Pits' pictures (TILES1.DAT to TILES3.DAT) and how a
// pit is drawn with them, as GFX.C LoadShapes and PITS.C Gdrall do.
// Works in a browser and in Node.

import { SIZE } from "./pit.mjs";

export const TILE = 24; // a square's size in pixels

// the palette the pits are drawn in (GFX.C SetPalette 1), 6 bits a colour
const PAL1 = [0,0,0, 63,63,63, 54,54,54, 50,50,50, 42,42,42, 37,37,37, 25,25,25, 63,0,0, 54,0,0, 46,0,0, 37,0,0, 63,63,0,
	63,42,0, 0,63,63, 29,46,63, 33,0,33, 33,63,33, 0,63,0, 0,46,0, 0,33,0, 0,33,63, 0,25,58, 0,16,50, 0,8,46,
	0,4,37, 0,0,33, 63,37,33, 46,21,21, 33,16,0, 29,12,0, 25,8,0, 21,4,0, 0,0,0, 31,31,31, 27,27,27, 25,25,25,
	21,21,21, 18,18,18, 12,12,12, 31,0,0, 27,0,0, 23,0,0, 18,0,0, 31,31,0, 31,21,0, 0,31,31, 14,23,31, 16,0,16,
	16,31,16, 0,31,0, 0,23,0, 0,16,0, 0,16,31, 0,12,29, 0,8,25, 0,4,23, 0,2,18, 0,0,16, 31,18,16, 23,10,10,
	16,8,0, 14,6,0, 12,4,0, 10,2,0];
// RGB by palette index (the VGA's 6 bits to 8)
export const PALETTE = Array.from({ length: 256 }, (_, i) =>
	i * 3 < PAL1.length ? [0, 1, 2].map((k) => (PAL1[i * 3 + k] << 2) | (PAL1[i * 3 + k] >> 4)) : [0, 0, 0]);

// the shapes in the tile files, in order (the game's shapes[]): { w, h,
// pixels (palette indexes) }; each file ends with a shape of width 0
export function readShapes(...files) {
	const shapes = [];
	for (const b of files) {
		for (let p = 0; b[p] !== 0 && p < b.length; ) {
			const w = b[p], h = b[p + 1];
			shapes.push({ w, h, pixels: b.subarray(p + 2, p + 2 + w * h) });
			p += w * h + 2;
		}
	}
	return shapes;
}

// draw a shape into an RGBA image (width iw) at x, y; with mask, colour 0
// is see-through (BlitMask)
export function blit(img, iw, shape, x, y, mask) {
	if (!shape) return;
	for (let j = 0; j < shape.h; j++) {
		for (let i = 0; i < shape.w; i++) {
			const c = shape.pixels[j * shape.w + i];
			if (mask && c === 0) continue;
			const px = x + i, py = y + j;
			if (px < 0 || py < 0 || px >= iw || py * iw * 4 >= img.length) continue;
			const o = (py * iw + px) * 4;
			const [r, g, b] = PALETTE[c];
			img[o] = r; img[o + 1] = g; img[o + 2] = b; img[o + 3] = 255;
		}
	}
}

// drawSquare draws the square at x, y of a level at px, py, as Gdrall:
// its ground, then the quarters of the objects on it and to its right
// and below, which stand half a square up and to the left
export function drawSquare(img, iw, shapes, level, x, y, px, py) {
	const at = (grid, a, b) => (a < SIZE && b < SIZE ? grid[a][b] : 0);
	const ground = level.under[x][y];
	blit(img, iw, shapes[ground || 0], px, py, false);
	const parts = [[at(level.over, x, y), 3], [at(level.over, x + 1, y), 2], [at(level.over, x, y + 1), 1], [at(level.over, x + 1, y + 1), 0]];
	for (const [tile, part] of parts) if (tile) blit(img, iw, shapes[tile + part], px, py, true);
}

// drawPit draws a whole level into a new RGBA image, SIZE squares a side
export function drawPit(shapes, level) {
	const iw = SIZE * TILE;
	const img = new Uint8ClampedArray(iw * iw * 4);
	for (let y = 0; y < SIZE; y++) for (let x = 0; x < SIZE; x++) drawSquare(img, iw, shapes, level, x, y, x * TILE, y * TILE);
	return { img, width: iw, height: iw };
}
