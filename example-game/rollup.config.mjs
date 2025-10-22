import resolve from '@rollup/plugin-node-resolve';
import commonjs from '@rollup/plugin-commonjs';
import typescript from 'rollup-plugin-typescript2';

export default {
  input: 'npm.bundle.ts',
  output: {
    file: '.godot/GodotJS/npm.bundle.js',
    format: 'iife',
    name: '__moduleExports__',
    exports: 'named',
    // Wrap the output in an AMD-style IIFE so it registers as an AMD module id 'npm'
    // Instrument define to log registration for debugging.
    banner: '(function(define){"use strict";\n' +
      'define("npm", ["require","exports"], function (require, exports) {',
    // Important: return the inner module exports so AMD loader receives them.
    footer: '\nreturn __moduleExports__;\n});\n' +
      '// restore original define to avoid leaking the logger beyond this bundle\n' +
     '})(define);'
  },
  plugins: [
    resolve({ extensions: ['.js', '.ts'] }),
    commonjs(),
    typescript({
      tsconfig: './tsconfig.bundle.json',
      tsconfigOverride: {
        include: ['npm.bundle.ts'],
        compilerOptions: { module: 'ES2022' }
      },
      clean: true,
    }),
  ],
};
