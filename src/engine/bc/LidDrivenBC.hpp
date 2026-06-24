#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// LidDrivenBC — applies the moving lid at j=ny-1
//
// Sets u = lid_vel on the top wall, u = 0 on all other walls.
// v = 0 everywhere on all walls.
//
// Apply separately to u and v:
//   lid_bc.apply_u(grid, u);
//   lid_bc.apply_v(grid, v);
// ─────────────────────────────────────────────────────────────────────────────

#include "../../../include/IGrid.hpp"
#include <vector>

class LidDrivenBC {
public:
    explicit LidDrivenBC(double lid_vel) : lid_vel_(lid_vel) {}

    void apply_u(const IGrid& g, std::vector<double>& u) const {
        int nx = g.nx(), ny = g.ny();
        // top lid: u = lid_vel
        for (int i = 0; i < nx; ++i) u[g.idx(i, ny-1)] = lid_vel_;
        // bottom, left, right: u = 0
        for (int i = 0; i < nx; ++i) u[g.idx(i, 0)]    = 0.0;
        for (int j = 0; j < ny; ++j) u[g.idx(0, j)]    = 0.0;
        for (int j = 0; j < ny; ++j) u[g.idx(nx-1, j)] = 0.0;
    }

    void apply_v(const IGrid& g, std::vector<double>& v) const {
        int nx = g.nx(), ny = g.ny();
        for (int i = 0; i < nx; ++i) { v[g.idx(i, 0)] = 0.0; v[g.idx(i, ny-1)] = 0.0; }
        for (int j = 0; j < ny; ++j) { v[g.idx(0, j)] = 0.0; v[g.idx(nx-1, j)] = 0.0; }
    }

    double lid_vel() const { return lid_vel_; }

private:
    double lid_vel_;
};
