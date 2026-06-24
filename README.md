# 2D Navier-Stokes & Thermal-Fluid Simulation Framework

A lightweight, high-performance 2D CFD and heat transfer solver built from scratch in **C++** with **zero external framework dependencies**. Implements incompressible fluid flow and conjugate heat transfer under a clean, extensible OOP architecture.

---

## Features

- **Incompressible Navier-Stokes Solver** — Chorin's Projection Method to decouple velocity and pressure fields
- **Thermal Energy Equation Solver** — Upwind differencing for advection, Central differencing for diffusion
- **Academic Validation** — Automated benchmark against **Ghia et al. (1982)**, with real-time $L_2$ and $L_\infty$ error norms
- **Real-Time Visualization** — SDL2-powered grid rendering, vector field plotting, and dynamic colormaps (Blue-Red, Diverging, Grayscale)
- **Interactive Scenarios** — Draggable rigid bodies (Dirichlet disks, Neumann obstacles) with real-time mouse interaction

---

## Numerical Methodology

### Chorin's Projection Method

The incompressible Navier-Stokes equations are solved via a 3-step fractional step approach:

**Step 1 — Intermediate Velocity ($u^*$)**  
Explicit time-stepping with Upwind advection and Central-difference diffusion.

**Step 2 — Pressure Poisson Equation**  
Solved iteratively via Gauss-Seidel to enforce incompressibility:

$$\nabla^2 p = \frac{1}{\Delta t} \nabla \cdot \mathbf{u}^*$$

**Step 3 — Velocity Correction**  
Divergence-free velocity field recovered from the pressure gradient:

$$\mathbf{u}^{n+1} = \mathbf{u}^* - \Delta t \, \nabla p$$

### Stability

Time step $\Delta t$ is checked against the CFL condition for diffusion-dominated flows:

$$\Delta t \le \frac{\Delta x^2}{4\alpha}$$

---

## Software Architecture

Strict unidirectional dependency flow: `Apps → Engine/Viz → Interfaces`

```
include/          ← Pure abstract interfaces (IGrid, IPDESolver, IBoundaryCondition, IRenderer)
src/engine/       ← Numerical solvers (NavierStokesSolver, PressureSolver, HeatSolver)
src/viz/          ← SDL2Renderer, colormaps, CSVExporter
apps/             ← Simulation entry points (cavity, heat2d, heat2d_disk, heat2d_obstacle)
```

Each simulation target has a `.cfg` config file — parameters can be changed without recompilation.

---

## Simulation Targets

| Target | Description |
|--------|-------------|
| `cavity` | Lid-driven cavity, Navier-Stokes via Chorin projection |
| `heat2d` | 2D heat diffusion with configurable boundary conditions |
| `heat2d_disk` | Heat diffusion with a draggable hot disk |
| `heat2d_obstacle` | Heat diffusion with a draggable insulating obstacle |

---

## Verification & Validation — Ghia et al. (1982)

Built-in `GhiaValidator` compares centerline velocity profiles against the reference benchmark.

**Current results (64×64 mesh, Re = 400):**

| Field | $L_2$ Error | $L_\infty$ Error |
|-------|-------------|-----------------|
| u-velocity (centerline) | 0.165 | 0.359 |
| v-velocity (centerline) | 0.225 | 0.409 |

> **Note:** Errors are primarily driven by spatial discretization at 64×64. A 128×128 grid with bilinear interpolation is planned to bring $L_2 < 0.05$.

---

## Build & Run

### Prerequisites

- **OS:** Linux (tested on Ubuntu)
- **Compiler:** GCC or Clang with C++17 support
- **Build system:** CMake ≥ 3.20
- **Libraries:** SDL2

### Compilation

```bash
git clone https://github.com/your-username/cfd-solver.git
cd cfd-solver
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### Run a simulation

```bash
./cavity
./heat2d
./heat2d_disk
./heat2d_obstacle
```

Config files are copied to the build directory automatically. Edit `.cfg` files to change Reynolds number, grid size, time step, and boundary conditions without recompiling.

---

## Roadmap

- [ ] 128×128 Ghia validation with bilinear interpolation
- [ ] Particle tracer for Lagrangian flow visualization
- [ ] Reynolds number sweep (Re = 100 → 3200)
- [ ] Geometry variations and additional boundary condition types
- [ ] OpenGL/GLSL shader-based rendering

---

## Background

This project is a self-directed portfolio piece targeting CFD solver/tool development roles in the automotive and aerospace sectors. The goal is to demonstrate genuine numerical methods knowledge and software engineering depth — not framework usage.

The solver is validated against the canonical Ghia et al. (1982) benchmark for lid-driven cavity flow, a standard test case in computational fluid dynamics literature.

---

## References

- Ghia, U., Ghia, K. N., & Shin, C. T. (1982). *High-Re solutions for incompressible flow using the Navier-Stokes equations and a multigrid method.* Journal of Computational Physics, 48(3), 387–411.
- Chorin, A. J. (1968). *Numerical solution of the Navier-Stokes equations.* Mathematics of Computation, 22(104), 745–762.
