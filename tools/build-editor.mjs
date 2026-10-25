// node tools/build-editor.mjs out.html [game-url]: the level editor as one
// HTML file, its code and data (the tiles, Abe's levels) inside, to use
// from the web or saved and offline. game-url is the web game Play opens
// (by default the one beside it, ../).
import fs from "node:fs";

const root = new URL("../", import.meta.url);
const read = (p) => fs.readFileSync(new URL(p, root));
const [out = "editor.html", game = "../"] = process.argv.slice(2);

// the modules as one: imports dropped, exports made plain
const code = ["tools/pit.mjs", "tools/tiles.mjs", "tools/rnd.mjs", "editor/editor.mjs"]
	.map((p) => read(p).toString().replace(/^import [^;]+;$/gm, "").replace(/^export /gm, ""))
	.join("\n");

const levels = {};
for (const name of fs.readdirSync(new URL("PITS95/LEVELS/", root)).filter((n) => /\.pit$/i.test(n)).sort()) {
	levels[name] = read("PITS95/LEVELS/" + name).toString("base64");
}
const data = {
	game,
	tiles: ["TILES1.DAT", "TILES2.DAT", "TILES3.DAT"].map((n) => read("PITS95/" + n).toString("base64")),
	levels,
};

const page = read("editor/index.html").toString()
	.replace("/*DATA*/", () => "const PITS_DATA = " + JSON.stringify(data) + ";")
	.replace("/*CODE*/", () => code);
fs.mkdirSync(new URL(".", new URL(out, "file://" + process.cwd() + "/")), { recursive: true });
fs.writeFileSync(out, page);
console.log(`${out}: ${(page.length / 1024).toFixed(0)} KB`);
