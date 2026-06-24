#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// IBoundaryCondition — Abstract boundary condition
//
// Each concrete BC knows which wall(s) it owns and what to write.
// apply() is called once per time step, after the interior update.
// ─────────────────────────────────────────────────────────────────────────────

#include <vector>
#include "IGrid.hpp"

class IBoundaryCondition {
public:
    virtual ~IBoundaryCondition() = default;

    // Write BC values into `field` (flat, row-major, size nx*ny).
    virtual void apply(const IGrid& grid, std::vector<double>& field) const = 0;
};
