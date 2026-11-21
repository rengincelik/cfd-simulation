// ─────────────────────────────────────────────────────────────────────────────
// cavity_wasm.cpp — browser build of the lid-driven cavity solver
//
// A thin C API over the existing solver headers. The solver itself is not
// modified: this file only creates it, steps it and exposes its fields.
// The page (docs/app.js) draws the fields and owns every control.
//
// Build:   see CMakeLists.txt (emcmake cmake -B build-wasm && cmake --build build-wasm)
// Output:  docs/cavity.js + docs/cavity.wasm
//
// Field layout is the solver's own: flat, row-major, index j*nx + i,
// row j = 0 at the bottom of the cavity.
// ─────────────────────────────────────────────────────────────────────────────

#include "../include/Config.hpp"
#include "../src/engine/NavierStokesSolver.hpp"
#include "../src/validation/GhiaValidator.hpp"
#include "../src/viz/Colormap.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <memory>
#include <vector>

#ifdef __EMSCRIPTEN__
  #include <emscripten/emscripten.h>
  #define CFD_API extern "C" EMSCRIPTEN_KEEPALIVE
#else
  #define CFD_API extern "C"   // native build, used for tests
#endif

namespace {

constexpr int MIN_N = 8;
constexpr int MAX_N = 256;

struct State {
    Config                              cfg;
    std::unique_ptr<NavierStokesSolver> solver;
    std::vector<std::uint8_t>           rgba;        // nx*ny*4, row 0 = top of the picture
    double                              stats[8]  = {0};
    double                              ghia[5]   = {0};
    Colormap                            cmap_vel{Colormap::Map::BLUE_RED};
    Colormap                            cmap_tmp{Colormap::Map::BLUE_RED};
    Colormap                            cmap_prs{Colormap::Map::DIVERGING};
};

State g;

bool ready() { return g.solver != nullptr; }

bool build() {
    try {
        const int nx = g.cfg.get_int("nx", 64);
        const int ny = g.cfg.get_int("ny", 64);
        if (nx < MIN_N || ny < MIN_N || nx > MAX_N || ny > MAX_N) {
            g.solver.reset();
            return false;
        }
        g.solver = std::make_unique<NavierStokesSolver>(g.cfg);
        g.rgba.assign(static_cast<std::size_t>(nx) * ny * 4, 0);
        return true;
    } catch (...) {
        g.solver.reset();
        return false;
    }
}

} // namespace

// ── Lifecycle ────────────────────────────────────────────────────────────────

// cfg_text uses the same "key value" format as cavity.cfg.
// Returns 1 on success, 0 if the text is invalid or the grid is out of range.
CFD_API int cfd_init(const char* cfg_text) {
    try {
        g.cfg = Config::from_string(cfg_text ? cfg_text : "");
    } catch (...) {
        g.solver.reset();
        return 0;
    }
    return build() ? 1 : 0;
}

CFD_API int cfd_reset() { return build() ? 1 : 0; }

// Advance n steps; returns the step count.
CFD_API int cfd_step(int n) {
    if (!ready()) return 0;
    for (int k = 0; k < n; ++k) g.solver->step();
    return g.solver->step_count();
}

// ── Grid ─────────────────────────────────────────────────────────────────────

CFD_API int    cfd_nx() { return ready() ? g.solver->grid().nx() : 0; }
CFD_API int    cfd_ny() { return ready() ? g.solver->grid().ny() : 0; }
CFD_API double cfd_lx() { return ready() ? g.solver->grid().lx() : 0.0; }
CFD_API double cfd_ly() { return ready() ? g.solver->grid().ly() : 0.0; }

// ── Fields ───────────────────────────────────────────────────────────────────

// which: 0 = u, 1 = v, 2 = p, 3 = temperature. Pointer to nx*ny doubles,
// valid until the next cfd_step / cfd_reset / cfd_init.
CFD_API const double* cfd_field(int which) {
    if (!ready()) return nullptr;
    switch (which) {
        case 0:  return g.solver->field_u().data();
        case 1:  return g.solver->field_v().data();
        case 2:  return g.solver->field_p().data();
        default: return g.solver->field_temp().data();
    }
}

// ── Picture ──────────────────────────────────────────────────────────────────

// mode: 0 = velocity magnitude, 1 = temperature, 2 = pressure
// Same normalisation and colormaps as SDL2Renderer. Returns nx*ny RGBA pixels;
// row 0 is the top of the cavity (the lid), so it can go straight into ImageData.
CFD_API const std::uint8_t* cfd_render_rgba(int mode) {
    if (!ready()) return nullptr;
    const auto& u = g.solver->field_u();
    const auto& v = g.solver->field_v();
    const auto& p = g.solver->field_p();
    const auto& t = g.solver->field_temp();
    const int nx = g.solver->grid().nx();
    const int ny = g.solver->grid().ny();

    double vmax = 1e-9, pmin = 1e9, pmax = -1e9;
    for (int k = 0; k < nx * ny; ++k) {
        vmax = std::max(vmax, std::sqrt(u[k] * u[k] + v[k] * v[k]));
        pmin = std::min(pmin, p[k]);
        pmax = std::max(pmax, p[k]);
    }

    for (int j = 0; j < ny; ++j) {
        for (int i = 0; i < nx; ++i) {
            const int k = j * nx + i;
            std::uint32_t argb;
            if (mode == 0) {
                argb = g.cmap_vel.argb(std::sqrt(u[k] * u[k] + v[k] * v[k]) / vmax);
            } else if (mode == 1) {
                argb = g.cmap_tmp.argb(t[k]);
            } else {
                argb = g.cmap_prs.argb(pmax > pmin ? (p[k] - pmin) / (pmax - pmin) : 0.5);
            }
            std::uint8_t* px = &g.rgba[(static_cast<std::size_t>(ny - 1 - j) * nx + i) * 4];
            px[0] = static_cast<std::uint8_t>(argb >> 16);
            px[1] = static_cast<std::uint8_t>(argb >> 8);
            px[2] = static_cast<std::uint8_t>(argb);
            px[3] = 255;
        }
    }
    return g.rgba.data();
}

// ── Numbers for the readout ──────────────────────────────────────────────────

// [0] step  [1] time  [2] max |Δu| of the last step  [3] max |u|
// [4] p min [5] p max [6] Reynolds  [7] 1 if every value is finite, else 0
CFD_API const double* cfd_stats() {
    if (!ready()) return nullptr;
    const auto& u = g.solver->field_u();
    const auto& v = g.solver->field_v();
    const auto& p = g.solver->field_p();
    double vmax = 0.0, pmin = 1e300, pmax = -1e300;
    bool finite = std::isfinite(g.solver->max_change());
    for (std::size_t k = 0; k < u.size(); ++k) {
        const double s = std::sqrt(u[k] * u[k] + v[k] * v[k]);
        if (!std::isfinite(s) || !std::isfinite(p[k])) { finite = false; break; }
        vmax = std::max(vmax, s);
        pmin = std::min(pmin, p[k]);
        pmax = std::max(pmax, p[k]);
    }
    g.stats[0] = g.solver->step_count();
    g.stats[1] = g.solver->sim_time();
    g.stats[2] = g.solver->max_change();
    g.stats[3] = finite ? vmax : 0.0;
    g.stats[4] = finite ? pmin : 0.0;
    g.stats[5] = finite ? pmax : 0.0;
    g.stats[6] = g.solver->reynolds();
    g.stats[7] = finite ? 1.0 : 0.0;
    return g.stats;
}

// ── Ghia et al. (1982) comparison ────────────────────────────────────────────

// [0] 1 if reference data exists for this Re (100, 400, 1000), else 0
// [1] u L2  [2] v L2  [3] u L∞  [4] v L∞
CFD_API const double* cfd_ghia() {
    for (double& x : g.ghia) x = 0.0;
    if (!ready()) return g.ghia;
    try {
        GhiaValidator val(static_cast<int>(g.solver->reynolds()));
        const auto r = val.compare(*g.solver);
        g.ghia[0] = 1.0;
        g.ghia[1] = r.u_l2;
        g.ghia[2] = r.v_l2;
        g.ghia[3] = r.u_linf;
        g.ghia[4] = r.v_linf;
    } catch (...) {
        g.ghia[0] = 0.0;
    }
    return g.ghia;
}
