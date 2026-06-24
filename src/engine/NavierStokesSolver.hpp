#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// NavierStokesSolver — 2-D incompressible Navier-Stokes
//
// Algorithm: Chorin projection method (fractional step)
//   Step 1: Intermediate velocity  u* = u^n + dt*(convection + diffusion)
//   Step 2: Pressure  ∇²p = (1/dt)*∇·u*    [PressureSolver, Gauss-Seidel]
//   Step 3: Velocity correction  u = u* - dt*∇p
//   Step 4: Energy equation      [HeatSolver]
//   Step 5: Apply all BCs
//
// Convection: first-order upwind
// Diffusion:  second-order central
//
// Usage:
//   Config cfg = Config::from_file("cavity.cfg");
//   NavierStokesSolver solver(cfg);
//   while (solver.sim_time() < t_end) solver.step();
// ─────────────────────────────────────────────────────────────────────────────

#include "../../include/IPDESolver.hpp"
#include "../../include/IGrid.hpp"
#include "../../include/Config.hpp"
#include "UniformGrid.hpp"
#include "PressureSolver.hpp"
#include "HeatSolver.hpp"
#include "bc/LidDrivenBC.hpp"
#include "bc/DirichletBC.hpp"
#include "bc/NeumannBC.hpp"

#include <vector>
#include <cmath>
#include <algorithm>
#include <memory>

class NavierStokesSolver : public IPDESolver {
public:
    explicit NavierStokesSolver(const Config& cfg)
        : grid_(cfg.get_int("nx", 64), cfg.get_int("ny", 64),
                cfg.get_double("lx", 1.0), cfg.get_double("ly", 1.0))
        , re_    (cfg.get_double("reynolds",   400.0))
        , dt_    (cfg.get_double("dt",         0.001))
        , p_iter_(cfg.get_int  ("p_iter",      50))
        , lid_vel_(cfg.get_double("lid_vel",   1.0))
        , pressure_solver_(p_iter_)
        , heat_solver_(re_, cfg.get_double("prandtl", 0.71))
        , lid_bc_(lid_vel_)
        , sim_time_(0.0)
        , step_count_(0)
        , max_change_(1.0)
    {
        int n = grid_.nx() * grid_.ny();
        u_.assign(n, 0.0);
        v_.assign(n, 0.0);
        p_.assign(n, 0.0);
        temp_.assign(n, 0.0);
        apply_bc();
    }

    // ── IPDESolver interface ──────────────────────────────────────────────────

    void step() override {
        std::vector<double> u_star, v_star;
        compute_intermediate(u_star, v_star);
        solve_pressure(u_star, v_star);
        correct_velocity(u_star, v_star);
        heat_solver_.step(grid_, dt_, u_, v_, temp_);
        apply_bc();

        // Track max change for convergence probe
        max_change_ = 0.0;
        for (int k = 0; k < (int)u_.size(); ++k) {
            double du = std::abs(u_[k] - u_star[k]);
            if (du > max_change_) max_change_ = du;
        }

        sim_time_  += dt_;
        step_count_ += 1;
    }

    const std::vector<double>& field_u()    const override { return u_; }
    const std::vector<double>& field_v()    const override { return v_; }
    const std::vector<double>& field_p()    const override { return p_; }
    const std::vector<double>& field_temp() const override { return temp_; }

    double sim_time()   const override { return sim_time_; }
    int    step_count() const override { return step_count_; }
    double max_change() const override { return max_change_; }

    // ── Extra accessors ───────────────────────────────────────────────────────

    const IGrid& grid()     const { return grid_; }
    double       reynolds() const { return re_; }
    double       dt()       const { return dt_; }

private:
    // ── Grid and sub-solvers ─────────────────────────────────────────────────
    UniformGrid     grid_;
    double          re_, dt_;
    int             p_iter_;
    double          lid_vel_;
    PressureSolver  pressure_solver_;
    HeatSolver      heat_solver_;
    LidDrivenBC     lid_bc_;

    // ── Fields ───────────────────────────────────────────────────────────────
    std::vector<double> u_, v_, p_, temp_;

    // ── State ────────────────────────────────────────────────────────────────
    double sim_time_;
    int    step_count_;
    double max_change_;

    // ── Chorin step 1: intermediate velocity ─────────────────────────────────
    void compute_intermediate(std::vector<double>& us,
                              std::vector<double>& vs) const
    {
        us = u_;
        vs = v_;

        double nu  = lid_vel_ / re_;
        double dx  = grid_.dx(), dy = grid_.dy();
        double dx2 = dx*dx, dy2 = dy*dy;
        int nx = grid_.nx(), ny = grid_.ny();

        for (int j = 1; j < ny-1; ++j) {
            for (int i = 1; i < nx-1; ++i) {
                double uc = u_[grid_.idx(i,j)];
                double vc = v_[grid_.idx(i,j)];

                // Upwind convection
                double dudx = (uc > 0)
                    ? (uc - u_[grid_.idx(i-1,j)]) / dx
                    : (u_[grid_.idx(i+1,j)] - uc) / dx;
                double dudy = (vc > 0)
                    ? (uc - u_[grid_.idx(i,j-1)]) / dy
                    : (u_[grid_.idx(i,j+1)] - uc) / dy;
                double dvdx = (uc > 0)
                    ? (vc - v_[grid_.idx(i-1,j)]) / dx
                    : (v_[grid_.idx(i+1,j)] - vc) / dx;
                double dvdy = (vc > 0)
                    ? (vc - v_[grid_.idx(i,j-1)]) / dy
                    : (v_[grid_.idx(i,j+1)] - vc) / dy;

                // Central diffusion
                double d2udx2 = (u_[grid_.idx(i+1,j)] - 2*uc + u_[grid_.idx(i-1,j)]) / dx2;
                double d2udy2 = (u_[grid_.idx(i,j+1)] - 2*uc + u_[grid_.idx(i,j-1)]) / dy2;
                double d2vdx2 = (v_[grid_.idx(i+1,j)] - 2*vc + v_[grid_.idx(i-1,j)]) / dx2;
                double d2vdy2 = (v_[grid_.idx(i,j+1)] - 2*vc + v_[grid_.idx(i,j-1)]) / dy2;

                us[grid_.idx(i,j)] = uc + dt_ * (-uc*dudx - vc*dudy + nu*(d2udx2 + d2udy2));
                vs[grid_.idx(i,j)] = vc + dt_ * (-uc*dvdx - vc*dvdy + nu*(d2vdx2 + d2vdy2));
            }
        }
    }

    // ── Chorin step 2: pressure ───────────────────────────────────────────────
    void solve_pressure(const std::vector<double>& us,
                        const std::vector<double>& vs)
    {
        int nx = grid_.nx(), ny = grid_.ny();
        double dx = grid_.dx(), dy = grid_.dy();
        std::vector<double> rhs(u_.size(), 0.0);

        for (int j = 1; j < ny-1; ++j)
            for (int i = 1; i < nx-1; ++i)
                rhs[grid_.idx(i,j)] = (1.0/dt_) * (
                    (us[grid_.idx(i+1,j)] - us[grid_.idx(i-1,j)]) / (2*dx) +
                    (vs[grid_.idx(i,j+1)] - vs[grid_.idx(i,j-1)]) / (2*dy)
                );

        pressure_solver_.solve(grid_, rhs, p_);
    }

    // ── Chorin step 3: velocity correction ───────────────────────────────────
    void correct_velocity(const std::vector<double>& us,
                          const std::vector<double>& vs)
    {
        int nx = grid_.nx(), ny = grid_.ny();
        double dx = grid_.dx(), dy = grid_.dy();

        for (int j = 1; j < ny-1; ++j) {
            for (int i = 1; i < nx-1; ++i) {
                u_[grid_.idx(i,j)] = us[grid_.idx(i,j)]
                    - dt_ * (p_[grid_.idx(i+1,j)] - p_[grid_.idx(i-1,j)]) / (2*dx);
                v_[grid_.idx(i,j)] = vs[grid_.idx(i,j)]
                    - dt_ * (p_[grid_.idx(i,j+1)] - p_[grid_.idx(i,j-1)]) / (2*dy);
            }
        }
    }

    // ── Apply all boundary conditions ─────────────────────────────────────────
    void apply_bc() {
        lid_bc_.apply_u(grid_, u_);
        lid_bc_.apply_v(grid_, v_);

        // Temperature: bottom hot, top cold (Dirichlet)
        DirichletBC bottom_hot (DirichletBC::BOTTOM, 1.0);
        DirichletBC top_cold   (DirichletBC::TOP,    0.0);
        bottom_hot.apply(grid_, temp_);
        top_cold.apply(grid_, temp_);
    }
};
