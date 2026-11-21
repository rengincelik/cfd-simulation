# 2D Navier–Stokes & Thermal-Fluid Simulation Framework

[**▶ Live Demo — Run the CFD Simulation in Your Browser**](https://rengincelik.github.io/cfd-simulation/)

A lightweight 2D Computational Fluid Dynamics (CFD) and heat transfer framework developed from scratch in **C++17**. The project implements incompressible fluid flow and thermal simulations using numerical methods, with an extensible object-oriented architecture.

The solver runs as a native desktop application using SDL2 and also supports browser-based execution through **WebAssembly (Emscripten)**.

---

## Features

- **Incompressible Navier–Stokes Solver** — Chorin's Projection Method for pressure-velocity coupling.
    
- **Thermal Energy Equation** — Upwind differencing for advection and central differencing for diffusion.
    
- **Academic Validation** — Automated comparison against Ghia et al. (1982), including (L_2) and (L_\infty) error norms.
    
- **Interactive Visualization** — SDL2-based rendering, vector fields, and dynamic colormaps.
    
- **Interactive Scenarios** — Draggable thermal objects and obstacles with configurable boundary conditions.
    
- **Browser Execution** — C++ solver compiled to WebAssembly, accessible through GitHub Pages.
    
- **Data Export** — CSV export of simulation fields.
    

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

## Numerical Methodology

### Chorin's Projection Method

The incompressible Navier–Stokes equations are solved using a three-step fractional-step approach.

**Step 1 — Intermediate Velocity ((u^*))**

Explicit time-stepping with upwind advection and central-difference diffusion.

**Step 2 — Pressure Poisson Equation**

Solved iteratively using Gauss–Seidel to enforce incompressibility:

[  
\nabla^2 p = \frac{1}{\Delta t}\nabla\cdot\mathbf{u}^*  
]

**Step 3 — Velocity Correction**

The divergence-free velocity field is recovered using the pressure gradient:

[  
\mathbf{u}^{n+1} = \mathbf{u}^*-\Delta t\nabla p  
]

### Stability

The time step is checked against the diffusion stability condition:

[  
\Delta t \leq \frac{\Delta x^2}{4\alpha}  
]

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

## Simulation Targets

|Target|Description|
|---|---|
|`cavity`|Lid-driven cavity flow using Chorin's projection method|
|`heat2d`|2D heat diffusion with configurable boundary conditions|
|`heat2d_disk`|Heat diffusion with a draggable hot disk|
|`heat2d_obstacle`|Heat diffusion with a draggable insulating obstacle|

---

## Verification & Validation — Ghia et al. (1982)

The built-in `GhiaValidator` compares the computed centerline velocity profiles against the reference benchmark for lid-driven cavity flow.

**Current results (64×64 mesh, Re = 400):**

|Field|(L_2) Error|(L_\infty) Error|
|---|---|---|
|u-velocity (centerline)|0.165|0.359|
|v-velocity (centerline)|0.225|0.409|

Errors are primarily influenced by spatial discretization at the current resolution.

---

## Build & Run — Native Application

### Prerequisites

- Linux (tested on Ubuntu)
    
- GCC or Clang with C++17 support
    
- CMake ≥ 3.16
    
- SDL2
    

### Compilation

```
git clone https://github.com/rengincelik/cfd-simulation.git
cd cfd-simulation

cmake -S . -B build
cmake --build build -j$(nproc)
```

### Run a Simulation

```
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

```
emcmake cmake -B build-wasm -DCMAKE_BUILD_TYPE=Release
cmake --build build-wasm -j$(nproc)
```

The generated `cavity.js` and `cavity.wasm` files are placed in `docs/`.

### Run Locally

```
python3 -m http.server -d docs 8000
```

Open `http://localhost:8000` in your browser.

---

## Roadmap

- Higher-resolution Ghia benchmark validation
    
- Particle tracer for Lagrangian flow visualization
    
- Reynolds number parameter sweep (Re = 100 → 3200)
    
- Additional geometries and boundary condition types
    
- Further visualization and rendering improvements
    

---

## Background

This project is a self-directed engineering project focused on CFD solver development, numerical methods and scientific computing.

The primary objective is to demonstrate an understanding of computational fluid dynamics and the implementation of numerical solvers from scratch, alongside software architecture and cross-platform execution.

The solver is benchmarked against the canonical lid-driven cavity results published by Ghia et al. (1982).

---

## References

- Ghia, U., Ghia, K. N., & Shin, C. T. (1982). _High-Re solutions for incompressible flow using the Navier-Stokes equations and a multigrid method._ Journal of Computational Physics, 48(3), 387–411.
    
- Chorin, A. J. (1968). _Numerical solution of the Navier-Stokes equations._ Mathematics of Computation, 22(104), 745–762.