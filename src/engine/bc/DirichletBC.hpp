#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// DirichletBC — fixed value on one or more walls
//
// Wall flags are ORed together:
//   DirichletBC bc(DirichletBC::BOTTOM | DirichletBC::TOP, 0.0);
// ─────────────────────────────────────────────────────────────────────────────

#include "../../../include/IBoundaryCondition.hpp"

class DirichletBC : public IBoundaryCondition {
public:
    enum Wall { BOTTOM = 1, TOP = 2, LEFT = 4, RIGHT = 8 };

    DirichletBC(int walls, double value) : walls_(walls), value_(value) {}

    void apply(const IGrid& g, std::vector<double>& f) const override {
        int nx = g.nx(), ny = g.ny();
        if (walls_ & BOTTOM) for (int i = 0; i < nx; ++i) f[g.idx(i, 0)]      = value_;
        if (walls_ & TOP)    for (int i = 0; i < nx; ++i) f[g.idx(i, ny-1)]   = value_;
        if (walls_ & LEFT)   for (int j = 0; j < ny; ++j) f[g.idx(0, j)]      = value_;
        if (walls_ & RIGHT)  for (int j = 0; j < ny; ++j) f[g.idx(nx-1, j)]   = value_;
    }

private:
    int    walls_;
    double value_;
};
