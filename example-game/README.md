# Using npm modules in a GodotJS game (example)

This document explains how to bundle npm packages into a single AMD module that GodotJS can load at startup, how to keep per-file TypeScript compilation for your game scripts, and how to consume the bundled module from your game code with short imports.

The instructions use generic names, so you can adapt them to your project and packages.

## Goal
- Bundle a set of npm packages (and their transitive dependencies) into one runtime file (for example: `npm.bundle.js`) that is evaluated by GodotJS on startup.
- Keep other TypeScript files compiled individually to `.godot/GodotJS/<your-script>.js` so Godot can load them per scene/script.
- Import the bundled npm APIs in your game code using a short module id, e.g. `import { something } from "npm";` or `const npm = require("npm");`.

## Overview of pieces
- `npm.bundle.ts` — an entry file that imports the npm packages you need and re-exports a compact public API.
- `rollup.config.mjs` — Rollup config that bundles `npm.bundle.ts` into `.godot/GodotJS/npm.bundle.js`, wrapped as an AMD module under a short id (e.g., `"npm"`).
- `typings/npm.bundle.d.ts` — a TypeScript declaration file that tells the TypeScript compiler (and your editor) that `"npm"` is a module and what it exports.

## 1) Example `npm.bundle.ts` (entry)
Create a small entry file that imports what you need from npm packages and exposes a clear exported API. Example:

```ts
// npm.bundle.ts
import somePackage from "some-package";
import { otherThing } from "other-package";

// Re-export a minimal API so game scripts import from a single id.
export function doStuff(...args: any[]) {
  return somePackage.process(...args);
}

export { otherThing };

// If you prefer a single default export:
// export default { doStuff, otherThing };
```

Keep this file minimal — it should serve as the single entry Rollup will bundle.

## 2) Rollup configuration
Use Rollup to bundle `npm.bundle.ts` and output an IIFE (or AMD) wrapped file that calls the runtime `define()` with a short id. Example relevant bits from `rollup.config.mjs` used in the example-game:

- Input: `npm.bundle.ts`
- Output file: `.godot/GodotJS/npm.bundle.js`
- We wrap the bundle to call `define("npm", ["require","exports"], factory)`, and the factory must return the module exports (or mutate the `exports` parameter).

Example minimal `rollup.config.mjs` snippet:

```js
import resolve from '@rollup/plugin-node-resolve';
import commonjs from '@rollup/plugin-commonjs';
import typescript from 'rollup-plugin-typescript2';

export default {
  input: 'npm.bundle.ts',
  output: {
    file: '.godot/GodotJS/npm.bundle.js',
    format: 'iife',
    name: '__moduleExports__',
    banner: '(function(define){"use strict"; define("npm", ["require","exports"], function (require, exports) {',
    footer: '\nreturn __moduleExports__;\n});\n})(define);'
  },
  plugins: [
    resolve({ extensions: ['.js', '.ts'] }),
    commonjs(),
    typescript({ tsconfig: './tsconfig.bundle.json' , clean: true}),
  ],
};
```

Notes:
- The outer IIFE receives the runtime `define` and uses it to register the AMD module id `"npm"`.
- The inner factory must either mutate the `exports` argument or return the exports object. The `footer` above returns the Rollup-generated `__moduleExports__` so the engine's AMD loader can pick it up.
- You can temporarily instrument (debug) `define` in this banner to log registrations; remove the instrumentation once debugging is done.

## 3) TypeScript typings
Create a `.d.ts` that declares the new module id so TypeScript and editors know about it. Put it under a `typings/` folder and ensure `tsconfig.json` includes it.

Example `typings/npm.bundle.d.ts`:

```ts
declare module "npm" {

  export function doStuff(...args: any[]): any;
  export { otherThing } from "other-package";
  //  export * from 'somepackage';
  // or declare a default export:
  // const _default: { doStuff(...args: any[]): any };
  // export default _default;
}
```

Then add `"typings"` (or the directory path) to the `typeRoots`/`include` in your `tsconfig.json` if needed so the compiler picks it up.

## 4) How to import from game scripts
After bundling and at runtime, the AMD module is registered with the id you chose (`"npm"` in this doc). Your per-file TypeScript game scripts can import it using either ESM-style or CommonJS-style depending on how your TS compiles to JS. Example using TypeScript `import` (preferred for editor/tooling):

```ts
// some_game_script.ts
import { doStuff } from "npm";

export class Something {
  ready() {
    console.log(doStuff(1,2,3));
  }
}
```

If your TS compiles to CommonJS-style require() calls at runtime, the compiled JS will call `require("npm")` and the GodotJS runtime's `require` will resolve the previously-registered `"npm"` AMD module.

If you prefer explicit require in runtime JS:

```js
const npm = require("npm");
console.log(npm.doStuff(1,2,3));
```

## 5) Build and run: quick steps
1. Install your npm dependencies in the project root (where `npm.bundle.ts` is):

```powershell
npm install some-package other-package
```

2. Build the bundle with Rollup (example assumes npm script `build` runs rollup):

```powershell
npm run build
```

This should produce `.godot/GodotJS/npm.bundle.js`.

3. Start Godot (or run the project). Make sure the bundle is evaluated before scripts that `require()` it run. Two common ways to do that:

- Add the bundle to `project.godot` so Godot preloads it at startup. Example (in `project.godot`):

```
runtime/core/preload_asm_modules=["res://.godot/GodotJS/npm.bundle.js"]
```

- Or point to the bundle from the editor UI: Project -> Project Settings -> GodotJS -> runtime -> Embedded Amd Modules and add the `.godot/GodotJS/npm.bundle.js` path there. Either option ensures Godot evaluates the AMD bundle early so `require("npm")` or equivalent resolves correctly.

4. From your per-file scripts, import from `"npm"` as shown above.

## tsconfig: prevent tsc from overwriting the Rollup bundle

Very important: your project's main `tsconfig.json` must NOT emit a `npm.bundle.js` that would overwrite the Rollup output. In this project the `tsconfig.json` already excludes the bundle entry. The relevant part looks like this:

```jsonc
{
  "compilerOptions": {
    "outDir": ".godot/GodotJS",
    // ... other options ...
  },
  "exclude": [
    "npm.bundle.ts"
  ]
}
```

Why: Rollup is responsible for producing the final `.godot/GodotJS/npm.bundle.js`. If `tsc` also compiled `npm.bundle.ts` it would write a JS file (and possibly overwrite Rollup's output). Excluding `npm.bundle.ts` from the main `tsc` build prevents that.

If you still want type-checking for the bundle entry, keep a separate `tsconfig.bundle.json` (used by Rollup's TypeScript plugin) and point the plugin at that config. This prevents `tsc --build` from accidentally writing bundle JS files into `.godot/GodotJS` while still allowing Rollup to use TypeScript types when bundling.

## 6) Debugging tips
- If an import shows an empty object ({}), either:
  - The AMD factory returned nothing and didn't mutate `exports`. Ensure your bundle returns the exports object or mutates `exports`.
  - The module was not registered under the id you expect — double-check the `define("id", ...)` id.
  - The bundle was evaluated too late; make sure embedded AMD modules are loaded before dependent scripts run.
- To confirm module registration order, temporarily instrument `define` in the `rollup` banner to `console.debug` registrations (remove later):

```js
banner: '(function(define){\nconst __orig_define__ = define;\ndefine = function(id, deps, factory){ console.debug("[AMD] define", id, deps); return __orig_define__.call(this,id,deps,factory); };\ndefine("npm", ["require","exports"], function(require,exports){',
```

- Check `globalThis.require.cache` from a script running in the same JS context to see which module ids are present:

```js
console.debug(Object.keys(globalThis.require.cache || {}));
```

## 7) Alternatives and tradeoffs
- Single flattened module vs multiple internal modules:
  - The approach above registers a single top-level AMD id (`"npm"`) which re-exports the APIs you want. This keeps imports short and explicit.
  - A fully flattened bundle removes internal module boundaries; this can simplify usage but may change initialization order and break some cyclic dependency patterns.

- Typings strategy:
  - If you only need a handful of functions from a large library, declare those in your `typings/npm.bundle.d.ts` rather than pulling the entire library's type definitions into your codebase.

## 8) Example project settings (optional)
In Godot you can ensure the bundle is evaluated at startup in one of two ways:

- Add the bundle to `project.godot` so it's preloaded by the engine (recommended for simplicity):

```
runtime/core/preload_asm_modules=["res://.godot/GodotJS/npm.bundle.js"]
```

- Or from the editor, go to Project -> Project Settings -> GodotJS -> runtime -> Embedded Amd Modules and add the path to `.godot/GodotJS/npm.bundle.js` there.

Either approach will load the AMD bundle early so modules defined inside it are available to scripts that `require()` them.

---

If you want, I can:
- Add a ready-to-use `npm.bundle.ts` example that re-exports a few commonly-used functions from a named package.
- Create a `tsconfig.bundle.json` example for bundling only the `npm.bundle.ts` entry.
- Add the `typings/npm.bundle.d.ts` to the `example-game` folder and wire the `tsconfig` to include it.

Tell me which extras you'd like and I'll add them. 
