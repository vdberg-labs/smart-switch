import { defineConfig } from "vite";

// Production build is served from GitHub Pages at
// https://vdberg-labs.github.io/smart-switch/config/ — local `npm run dev` stays at `/`.
export default defineConfig(({ command }) => ({
	base: command === "build" ? "/smart-switch/config/" : "/",
}));
