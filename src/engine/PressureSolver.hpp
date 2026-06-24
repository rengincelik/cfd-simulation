#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// PressureSolver — solves ∇²p = rhs via Gauss-Seidel iteration
//
// Called by NavierStokesSolver at each projection step.
// Pressure BC: Neumann (∂p/∂n = 0) on all walls.
// ─────────────────────────────────────────────────────────────────────────────

#include <vector>
#include "../../include/IGrid.hpp"

class PressureSolver {
public:
    // max_iter : Gauss-Seidel iteration cap per time step
    explicit PressureSolver(int max_iter = 50) : max_iter_(max_iter) {}

    // Solve in-place. rhs must be pre-computed as (1/dt)*div(u*).
    // p is updated; Neumann BC enforced each iteration.
    void solve(const IGrid& g,
               const std::vector<double>& rhs,
               std::vector<double>& p) const
    {
        int nx = g.nx(), ny = g.ny();
        double dx2 = g.dx() * g.dx();
        double dy2 = g.dy() * g.dy();
        double denom = 2.0 / dx2 + 2.0 / dy2;

        for (int iter = 0; iter < max_iter_; ++iter) {
            for (int j = 1; j < ny-1; ++j) {
                for (int i = 1; i < nx-1; ++i) {
                    p[g.idx(i,j)] = (
                        (p[g.idx(i+1,j)] + p[g.idx(i-1,j)]) / dx2 +
                        (p[g.idx(i,j+1)] + p[g.idx(i,j-1)]) / dy2 -
                        rhs[g.idx(i,j)]
                    ) / denom;
                }
            }
            // Neumann BC on all walls
            apply_neumann(g, p);
        }
    }

private:
    int max_iter_;

    static void apply_neumann(const IGrid& g, std::vector<double>& p) {
        int nx = g.nx(), ny = g.ny();
        for (int i = 0; i < nx; ++i) {
            p[g.idx(i, 0)]    = p[g.idx(i, 1)];
            p[g.idx(i, ny-1)] = p[g.idx(i, ny-2)];
        }
        for (int j = 0; j < ny; ++j) {
            p[g.idx(0, j)]    = p[g.idx(1, j)];
            p[g.idx(nx-1, j)] = p[g.idx(nx-2, j)];
        }
    }
};
