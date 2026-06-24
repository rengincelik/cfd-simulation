#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// IGrid — Abstract 2D uniform grid
//
// Owns spatial metadata only (size, spacing).
// Field data (u, v, p, T …) lives in the solver or a separate Field object.
// ─────────────────────────────────────────────────────────────────────────────

class IGrid {
public:
    virtual ~IGrid() = default;

    virtual int    nx() const = 0;   // number of cells / nodes in x
    virtual int    ny() const = 0;   // number of cells / nodes in y
    virtual double dx() const = 0;   // cell width
    virtual double dy() const = 0;   // cell height
    virtual double lx() const = 0;   // domain width  (= (nx-1)*dx)
    virtual double ly() const = 0;   // domain height (= (ny-1)*dy)

    // Row-major flat index: (i=col, j=row) → j*nx + i
    int idx(int i, int j) const { return j * nx() + i; }
};
