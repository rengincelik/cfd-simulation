#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// UniformGrid — uniform 2-D Cartesian grid
// ─────────────────────────────────────────────────────────────────────────────

#include "../../include/IGrid.hpp"

class UniformGrid : public IGrid {
public:
    // nx_ × ny_ nodes, domain [0,lx_] × [0,ly_]
    UniformGrid(int nx, int ny, double lx = 1.0, double ly = 1.0)
        : nx_(nx), ny_(ny), lx_(lx), ly_(ly),
          dx_(lx / (nx - 1)), dy_(ly / (ny - 1)) {}

    int    nx() const override { return nx_; }
    int    ny() const override { return ny_; }
    double dx() const override { return dx_; }
    double dy() const override { return dy_; }
    double lx() const override { return lx_; }
    double ly() const override { return ly_; }

private:
    int    nx_, ny_;
    double lx_, ly_, dx_, dy_;
};
