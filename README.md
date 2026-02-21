# React + Vite

This template provides a minimal setup to get React working in Vite with HMR and some ESLint rules.

Currently, two official plugins are available:

- [@vitejs/plugin-react](https://github.com/vitejs/vite-plugin-react/blob/main/packages/plugin-react) uses [Babel](https://babeljs.io/) (or [oxc](https://oxc.rs) when used in [rolldown-vite](https://vite.dev/guide/rolldown)) for Fast Refresh
- [@vitejs/plugin-react-swc](https://github.com/vitejs/vite-plugin-react/blob/main/packages/plugin-react-swc) uses [SWC](https://swc.rs/) for Fast Refresh

## React Compiler

The React Compiler is not enabled on this template because of its impact on dev & build performances. To add it, see [this documentation](https://react.dev/learn/react-compiler/installation).

## Expanding the ESLint configuration

If you are developing a production application, we recommend using TypeScript with type-aware lint rules enabled. Check out the [TS template](https://github.com/vitejs/vite/tree/main/packages/create-vite/template-react-ts) for information on how to integrate TypeScript and [`typescript-eslint`](https://typescript-eslint.io) in your project.
# equation-solver

#### Compile to WebAssembly
- Install the Emscripten SDK (emsdk). Once installed, run this command in your terminal to compile the C++ file into WebAssembly:
```
emcc solver.cpp -o solver.js -lembind -s MODULARIZE=1 -s EXPORT_NAME="createSolver"
```
- This generates two files: solver.wasm (the binary) and solver.js (the glue code that lets React talk to the binary).
```
npm create vite@latest equation-solver -- --template react
cd equation-solver && npm install
npm install react-d3-tree
```
- Move the Wasm files: Copy the solver.js and solver.wasm you generated in Step 2 into your React app's public/ directory.