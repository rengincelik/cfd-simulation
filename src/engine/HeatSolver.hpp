#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// HeatSolver — advances the energy equation one time step
//
//   ∂T/∂t + u∂T/∂x + v∂T/∂y = α∇²T
//
//   α = 1 / (Re * Pr)   (non-dimensional, Pr = 0.71 for air by default)
//
// Convection: first-order upwind (stable for any Pe)
// Diffusion:  central difference
// Time:       explicit forward Euler
// ─────────────────────────────────────────────────────────────────────────────

#include "../../include/IGrid.hpp"
#include <vector>

class HeatSolver {
public:
    // re  — Reynolds number (passed in from NavierStokesSolver)
    // pr  — Prandtl number (default 0.71 = air)
    HeatSolver(double re, double pr = 0.71) : alpha_(1.0 / (re * pr)) {}

    // Advance temp by dt; u, v are the corrected velocity fields.
    // T_bc is applied by the caller after this returns.
    void step(const IGrid& g,
              double dt,
              const std::vector<double>& u,
              const std::vector<double>& v,
              std::vector<double>& temp) const
    {
        int nx = g.nx(), ny = g.ny();
        double dx = g.dx(), dy = g.dy();
        double dx2 = dx*dx, dy2 = dy*dy;

        std::vector<double> t_new = temp;

        for (int j = 1; j < ny-1; ++j) {
            for (int i = 1; i < nx-1; ++i) {
                double T  = temp[g.idx(i,j)];
                double uc = u[g.idx(i,j)];
                double vc = v[g.idx(i,j)];

                // Upwind convection
                double dTdx = (uc > 0)
                    ? (T - temp[g.idx(i-1,j)]) / dx
                    : (temp[g.idx(i+1,j)] - T) / dx;
                double dTdy = (vc > 0)
                    ? (T - temp[g.idx(i,j-1)]) / dy
                    : (temp[g.idx(i,j+1)] - T) / dy;

                // Central diffusion
                double d2Tdx2 = (temp[g.idx(i+1,j)] - 2*T + temp[g.idx(i-1,j)]) / dx2;
                double d2Tdy2 = (temp[g.idx(i,j+1)] - 2*T + temp[g.idx(i,j-1)]) / dy2;

                t_new[g.idx(i,j)] = T + dt * (
                    -uc*dTdx - vc*dTdy
                    + alpha_ * (d2Tdx2 + d2Tdy2)
                );
            }
        }
        temp = std::move(t_new);
    }

    double alpha() const { return alpha_; }

private:
    double alpha_;
};
