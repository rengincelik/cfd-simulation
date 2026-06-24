#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// NeumannBC — zero normal gradient: ∂φ/∂n = 0
//
// Ghost-cell extrapolation: boundary node = first interior node.
// ─────────────────────────────────────────────────────────────────────────────

#include "../../../include/IBoundaryCondition.hpp"

class NeumannBC : public IBoundaryCondition {
public:
    enum Wall { BOTTOM = 1, TOP = 2, LEFT = 4, RIGHT = 8 };

    explicit NeumannBC(int walls) : walls_(walls) {}

    void apply(const IGrid& g, std::vector<double>& f) const override {
        int nx = g.nx(), ny = g.ny();
        if (walls_ & BOTTOM) for (int i = 0; i < nx; ++i) f[g.idx(i, 0)]    = f[g.idx(i, 1)];
        if (walls_ & TOP)    for (int i = 0; i < nx; ++i) f[g.idx(i, ny-1)] = f[g.idx(i, ny-2)];
        if (walls_ & LEFT)   for (int j = 0; j < ny; ++j) f[g.idx(0, j)]    = f[g.idx(1, j)];
        if (walls_ & RIGHT)  for (int j = 0; j < ny; ++j) f[g.idx(nx-1, j)] = f[g.idx(nx-2, j)];
    }

private:
    int walls_;
};
