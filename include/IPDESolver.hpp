#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// IPDESolver — Abstract time-stepping PDE solver (partial diferantial equation)
//
// Owns the field state internally.
// Caller drives the loop: while (!done) { solver.step(); }
// ─────────────────────────────────────────────────────────────────────────────

#include <vector>

class IPDESolver {
public:
    virtual ~IPDESolver() = default;

    // Advance solution by one time step dt.
    virtual void step() = 0;

    // Read-only access to internal field(s) for visualisation / output.
    // Returns a flat row-major array of size nx*ny.
    virtual const std::vector<double>& field_u()    const = 0;
    virtual const std::vector<double>& field_v()    const = 0;
    virtual const std::vector<double>& field_p()    const = 0;
    virtual const std::vector<double>& field_temp() const = 0;

    // Current simulated time and step count.
    virtual double sim_time()  const = 0;
    virtual int    step_count() const = 0;

    // Max absolute change between the last two steps (convergence probe).
    virtual double max_change() const = 0;
};
