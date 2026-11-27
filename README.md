# C++ Simulation Engine — 2D Fluid Flow & Heat Transfer

[**▶ Live Demo — run it in your browser**](https://rengincelik.github.io/cfd-simulation/)

A from-scratch **C++17** engine that simulates 2D incompressible fluid flow and heat transfer. The same solver code builds into native desktop apps (SDL2) and, through **Emscripten**, into a WebAssembly module that runs in the browser. There is no separate JavaScript version of the solver.

*Status: in progress. The core solver, browser build and benchmark validation at 64×64 are done; higher-resolution validation is ongoing (see [Validation](#verification--validation--ghia-et-al-1982)).*

---

## Highlights

- **Interface-based architecture** — abstract `IGrid`, `IPDESolver` and `IBoundaryCondition` interfaces with a one-way dependency flow: `apps → engine / visualization → interfaces`. Solvers know nothing about rendering.
- **One codebase, two targets** — native SDL2 apps and a WebAssembly build. The browser talks to the unchanged C++ solver through a small C API (`web/cavity_wasm.cpp`).
- **Config-driven runs** — every app reads a `.cfg` file, so parameters change without recompiling.
- **Cache-friendly data layout** — fields are flat, contiguous `std::vector<double>` buffers with row-major indexing.
- **Automated validation** — a built-in validator compares results with a published benchmark (Ghia et al. 1982) and reports $L_2$ / $L_\infty$ error norms.
- **Interactive** — real-time visualization with colormaps and vector fields, draggable objects, CSV export.
- **No external frameworks** — standard library + SDL2 for the native window; no third-party libraries in the WASM build.

---

## Software Architecture

The project follows a modular architecture with a unidirectional dependency flow:

`Apps → Engine / Visualization → Interfaces`

```
cfd-simulation/
├── apps/                 # Native simulation entry points
├── include/              # Abstract interfaces and configuration
├── src/
│   ├── engine/           # Numerical solvers
│   ├── validation/       # Ghia benchmark validation
│   └── viz/              # SDL2 rendering, colormaps and CSV export
├── web/                  # WebAssembly C API
├── docs/                 # GitHub Pages browser application
│   ├── index.html
│   ├── style.css
│   ├── app.js
│   ├── cavity.js
│   └── cavity.wasm
├── CMakeLists.txt
└── *.cfg                 # Simulation configurations
```

Each native simulation target has its own `.cfg` configuration file. Parameters can be modified without recompiling the application.


---

## Browser Demo — WebAssembly

The lid-driven cavity simulation can run directly in a web browser without installing any software.

[**Open the Live Simulation**](https://rengincelik.github.io/cfd-simulation/)

The browser version uses the original C++ numerical solver compiled to WebAssembly with Emscripten. It does not use a separate JavaScript implementation of the solver.

### Interactive Controls

- **Simulation Control:** Run, pause and reset.
- **Velocity Field:** Visualize velocity magnitude and vector directions.
- **Temperature Field:** Inspect the temperature distribution.
- **Pressure Field:** Visualize the pressure field.
- **Reynolds Number:** Select Re = 100, 400 or 1000.
- **Grid Resolution:** Select 32×32, 64×64 or 128×128.
- **Simulation Speed:** Adjust the number of solver steps per frame.
- **Live Readouts:** Monitor simulation time, velocity, changes and pressure range.
- **Ghia Validation:** Compare centerline velocity profiles against Ghia et al. (1982).
- **CSV Export:** Export simulation fields for further analysis.

Keyboard shortcuts:

- `Space` — Pause / Resume
- `R` — Reset
- `1`, `2`, `3` — Switch visualization

### Browser Architecture

The browser implementation reuses the existing C++ solver and exposes its functionality through a lightweight C API.

|Component|Responsibility|
|---|---|
|`web/cavity_wasm.cpp`|C API for solver initialization, execution and data access|
|`docs/index.html`|Browser interface|
|`docs/style.css`|Interface styling|
|`docs/app.js`|Simulation controls and canvas rendering|
|`docs/cavity.js`|Emscripten-generated JavaScript module|
|`docs/cavity.wasm`|Compiled WebAssembly module|

GitHub Pages serves the browser application directly from the `docs/` directory.


---

## Build & Run — Native Application

### Prerequisites

- Linux (tested on Ubuntu)
- GCC or Clang with C++17 support
- CMake ≥ 3.16
- SDL2

### Compilation

```sh
git clone https://github.com/rengincelik/cfd-simulation.git
cd cfd-simulation

cmake -S . -B build
cmake --build build -j$(nproc)
```

### Run a Simulation

```sh
cd build

./cavity
./heat2d
./heat2d_disk
./heat2d_obstacle
```

Configuration files are automatically copied into the build directory. Edit the corresponding `.cfg` files to change Reynolds number, grid resolution, time step and boundary conditions.


---

## Build — WebAssembly

The browser version requires [Emscripten](https://emscripten.org/) and does not require SDL2.

### Compile

```sh
emcmake cmake -B build-wasm -DCMAKE_BUILD_TYPE=Release
cmake --build build-wasm -j$(nproc)
```

The generated `cavity.js` and `cavity.wasm` files are placed in `docs/`.

### Run Locally

```sh
python3 -m http.server -d docs 8000
```

Open `http://localhost:8000` in your browser.


---

## Simulation Targets

|Target|Description|
|---|---|
|`cavity`|Lid-driven cavity flow using Chorin's projection method|
|`heat2d`|2D heat diffusion with configurable boundary conditions|
|`heat2d_disk`|Heat diffusion with a draggable hot disk|
|`heat2d_obstacle`|Heat diffusion with a draggable insulating obstacle|


---

## Numerical Methodology

### Chorin's Projection Method

The incompressible Navier–Stokes equations are solved using a three-step fractional-step approach.

**Step 1 — Intermediate Velocity ($u^*$)**

Explicit time-stepping with upwind advection and central-difference diffusion.

**Step 2 — Pressure Poisson Equation**

Solved iteratively using Gauss–Seidel to enforce incompressibility:

```math
\nabla^2 p = \frac{1}{\Delta t}\nabla\cdot\mathbf{u}^*
```

**Step 3 — Velocity Correction**

The divergence-free velocity field is recovered using the pressure gradient:

```math
\mathbf{u}^{n+1} = \mathbf{u}^*-\Delta t\nabla p
```

### Stability

The time step is checked against the diffusion stability condition:

```math
\Delta t \leq \frac{\Delta x^2}{4\alpha}
```


---

## Verification & Validation — Ghia et al. (1982)

The built-in `GhiaValidator` compares the computed centerline velocity profiles against the reference benchmark for lid-driven cavity flow.

**Current results (64×64 mesh, Re = 400):**

|Field|$L_2$ Error|$L_\infty$ Error|
|---|---|---|
|u-velocity (centerline)|0.165|0.359|
|v-velocity (centerline)|0.225|0.409|

Errors are primarily influenced by spatial discretization at the current resolution.


---

## Roadmap

- Higher-resolution Ghia benchmark validation
- Particle tracer for Lagrangian flow visualization
- Reynolds number parameter sweep (Re = 100 → 3200)
- Additional geometries and boundary condition types
- Further visualization and rendering improvements


---

## Background

A self-directed project. I wanted to see whether I could take a numerical method from my engineering degree and turn it into a properly structured, testable program, and then ship that same program to the browser without rewriting it.

The physics side is a classic: Chorin's projection method for incompressible flow, checked against the lid-driven cavity results of Ghia et al. (1982). The software side is the part I keep investing in: clean interfaces, one codebase for native and web, and validation that runs as code rather than by eye.

---

## References

- Ghia, U., Ghia, K. N., & Shin, C. T. (1982). _High-Re solutions for incompressible flow using the Navier-Stokes equations and a multigrid method._ Journal of Computational Physics, 48(3), 387–411.
- Chorin, A. J. (1968). _Numerical solution of the Navier-Stokes equations._ Mathematics of Computation, 22(104), 745–762.
