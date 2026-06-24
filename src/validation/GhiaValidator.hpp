#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// GhiaValidator — compares centerline velocity profiles to Ghia et al. 1982
//
// Ghia, U., Ghia, K.N., Shin, C.T. (1982).
// High-Re solutions for incompressible flow using the Navier-Stokes equations
// and a multigrid method. J. Comput. Phys. 48, 387-411.
//
// Available reference Re: 100, 400, 1000
//
// Usage:
//   GhiaValidator val(400);
//   auto result = val.compare(solver);
//   std::cout << "u-centerline L2 error: " << result.u_l2 << "\n";
// ─────────────────────────────────────────────────────────────────────────────

#include "../engine/NavierStokesSolver.hpp"
#include <vector>
#include <cmath>
#include <stdexcept>
#include <string>
#include <iostream>
#include <iomanip>

class GhiaValidator {
public:
    struct Result {
        double u_l2;     // L2 error on u along vertical centerline
        double v_l2;     // L2 error on v along horizontal centerline
        double u_linf;   // L∞ error on u
        double v_linf;   // L∞ error on v
    };

    explicit GhiaValidator(int re) {
        switch (re) {
            case 100:  load_re100();  break;
            case 400:  load_re400();  break;
            case 1000: load_re1000(); break;
            default:
                throw std::runtime_error(
                    "GhiaValidator: no reference data for Re=" + std::to_string(re));
        }
    }

    // Compare solver fields to reference data.
    // Interpolates solver result to Ghia's sample points.
    Result compare(const NavierStokesSolver& solver) const {
        const auto& u    = solver.field_u();
        const auto& v    = solver.field_v();
        const IGrid& g   = solver.grid();
        int nx = g.nx(), ny = g.ny();

        // u along vertical centerline x=0.5, varying y
        double u_l2 = 0.0, u_linf = 0.0;
        int i_mid = nx / 2;
        for (int k = 0; k < (int)ghia_y_.size(); ++k) {
            double y   = ghia_y_[k];
            double u_r = ghia_u_[k];
            // Nearest-node interpolation (sufficient for coarse validation)
            int j = (int)std::round(y * (ny - 1));
            j = std::max(0, std::min(ny-1, j));
            double u_s = u[g.idx(i_mid, j)];
            double e = std::abs(u_s - u_r);
            u_l2  += e * e;
            if (e > u_linf) u_linf = e;
        }
        u_l2 = std::sqrt(u_l2 / ghia_y_.size());

        // v along horizontal centerline y=0.5, varying x
        double v_l2 = 0.0, v_linf = 0.0;
        int j_mid = ny / 2;
        for (int k = 0; k < (int)ghia_x_.size(); ++k) {
            double x   = ghia_x_[k];
            double v_r = ghia_v_[k];
            int i = (int)std::round(x * (nx - 1));
            i = std::max(0, std::min(nx-1, i));
            double v_s = v[g.idx(i, j_mid)];
            double e = std::abs(v_s - v_r);
            v_l2  += e * e;
            if (e > v_linf) v_linf = e;
        }
        v_l2 = std::sqrt(v_l2 / ghia_x_.size());

        return {u_l2, v_l2, u_linf, v_linf};
    }

    // Print a formatted comparison table to stdout.
    void print_report(const NavierStokesSolver& solver) const {
        auto r = compare(solver);
        std::cout << "\n── Ghia et al. 1982 Validation ──────────────────\n";
        std::cout << std::fixed << std::setprecision(6);
        std::cout << "  u centerline  L2=" << r.u_l2
                  << "  Linf=" << r.u_linf << "\n";
        std::cout << "  v centerline  L2=" << r.v_l2
                  << "  Linf=" << r.v_linf << "\n";
        std::cout << "─────────────────────────────────────────────────\n\n";
    }

private:
    // Reference points: y positions [0,1] and u-velocity at x=0.5
    std::vector<double> ghia_y_, ghia_u_;
    // Reference points: x positions [0,1] and v-velocity at y=0.5
    std::vector<double> ghia_x_, ghia_v_;

    // ── Re=100 ───────────────────────────────────────────────────────────────
    void load_re100() {
        ghia_y_ = {0.0000,0.0547,0.0625,0.0703,0.1016,0.1719,0.2813,
                   0.4531,0.5000,0.6172,0.7344,0.8516,0.9531,0.9609,
                   0.9688,0.9766,1.0000};
        ghia_u_ = {0.00000,-0.03717,-0.04192,-0.04775,-0.06434,-0.10150,
                   -0.15662,-0.21090,-0.20581,-0.13641, 0.00332, 0.23111,
                    0.68717, 0.73722, 0.78871, 0.84123, 1.00000};

        ghia_x_ = {0.0000,0.0625,0.0703,0.0781,0.0938,0.1563,0.2266,
                   0.2344,0.5000,0.8047,0.8594,0.9063,0.9453,0.9531,
                   0.9609,0.9688,1.0000};
        ghia_v_ = {0.00000, 0.09233, 0.10091, 0.10890, 0.12326, 0.16256,
                    0.17622, 0.17903, 0.05454,-0.24533,-0.22445,-0.16914,
                   -0.10313,-0.08864,-0.07391,-0.05906, 0.00000};
    }

    // ── Re=400 ───────────────────────────────────────────────────────────────
    void load_re400() {
        ghia_y_ = {0.0000,0.0547,0.0625,0.0703,0.1016,0.1719,0.2813,
                   0.4531,0.5000,0.6172,0.7344,0.8516,0.9531,0.9609,
                   0.9688,0.9766,1.0000};
        ghia_u_ = {0.00000,-0.08186,-0.09266,-0.10338,-0.14612,-0.24299,
                   -0.32726,-0.17119,-0.11477, 0.02135, 0.16256, 0.29093,
                    0.55892, 0.61756, 0.68439, 0.75837, 1.00000};

        ghia_x_ = {0.0000,0.0625,0.0703,0.0781,0.0938,0.1563,0.2266,
                   0.2344,0.5000,0.8047,0.8594,0.9063,0.9453,0.9531,
                   0.9609,0.9688,1.0000};
        ghia_v_ = {0.00000, 0.18360, 0.19713, 0.20920, 0.22965, 0.28124,
                    0.30203, 0.30174, 0.05186,-0.38598,-0.44993,-0.38598,
                   -0.23827,-0.22847,-0.19254,-0.15663, 0.00000};
    }

    // ── Re=1000 ──────────────────────────────────────────────────────────────
    void load_re1000() {
        ghia_y_ = {0.0000,0.0547,0.0625,0.0703,0.1016,0.1719,0.2813,
                   0.4531,0.5000,0.6172,0.7344,0.8516,0.9531,0.9609,
                   0.9688,0.9766,1.0000};
        ghia_u_ = {0.00000,-0.18109,-0.20196,-0.22220,-0.29730,-0.38289,
                   -0.27805,-0.10648,-0.06080, 0.05702, 0.18719, 0.33304,
                    0.46413, 0.51550, 0.57492, 0.65928, 1.00000};

        ghia_x_ = {0.0000,0.0625,0.0703,0.0781,0.0938,0.1563,0.2266,
                   0.2344,0.5000,0.8047,0.8594,0.9063,0.9453,0.9531,
                   0.9609,0.9688,1.0000};
        ghia_v_ = {0.00000, 0.27485, 0.29012, 0.30353, 0.32627, 0.37095,
                    0.33075, 0.32235, 0.02526,-0.31966,-0.42665,-0.51550,
                   -0.39188,-0.33714,-0.27669,-0.21388, 0.00000};
    }
};
