// ─────────────────────────────────────────────────────────────────────────────
// heat2d_main.cpp — 2D Heat Diffusion
//
// BC: sol sıcak (T=1), sağ soğuk (T=0), üst/alt yalıtılmış (Neumann)
//
// Tuşlar:
//   Space → durdur / devam
//   R     → sıfırla
//   Esc   → çık
// ─────────────────────────────────────────────────────────────────────────────

#include "../include/Config.hpp"
#include "../src/engine/UniformGrid.hpp"
#include "../src/engine/bc/DirichletBC.hpp"
#include "../src/engine/bc/NeumannBC.hpp"
#include "../src/viz/SDL2Renderer.hpp"
#include "../src/viz/Colormap.hpp"

#include <SDL2/SDL.h>
#include <vector>
#include <algorithm>
#include <cmath>
#include <cstdio>



int main(int argc, char* argv[]) {
    const char* cfg_path = (argc > 1) ? argv[1] : "heat2d.cfg";
    Config cfg = Config::from_file(cfg_path);


    int    nx  = cfg.get_int   ("nx",       128);
    int    ny  = cfg.get_int   ("ny",       128);
    double alpha = cfg.get_double("alpha",  0.01);
    int    win = cfg.get_int   ("win_size", 640);
    int    spf = cfg.get_int   ("steps_per_frame", 20);

    UniformGrid grid(nx, ny);
    double dx2 = grid.dx() * grid.dx();
    double dy2 = grid.dy() * grid.dy();
    double dt  = 0.24 * std::min(dx2, dy2) / alpha;
    double rx  = alpha * dt / dx2;
    double ry  = alpha * dt / dy2;

    printf("Grid: %dx%d  dx=%.4f  dt=%.6f  r=%.4f\n",
           nx, ny, grid.dx(), dt, rx);

    // BCs
    DirichletBC left_hot (DirichletBC::LEFT,  1.0);
    DirichletBC right_cold(DirichletBC::RIGHT, 0.0);
    NeumannBC   insulated(NeumannBC::BOTTOM | NeumannBC::TOP);

    auto apply_bc = [&](std::vector<double>& T) {
        left_hot.apply(grid, T);
        right_cold.apply(grid, T);
        insulated.apply(grid, T);
    };

    std::vector<double> T    (nx*ny, 0.0);
    std::vector<double> T_new(nx*ny, 0.0);
    apply_bc(T);

    // Renderer — reuse velocity slot for temperature display
    SDL2Renderer renderer(nx, ny, win, 0);
    std::vector<double> zero(nx*ny, 0.0);
    Colormap cmap;

    bool paused    = false;
    bool converged = false;
    int  frame     = 0;

    while (renderer.is_open()) {
        if (!renderer.poll_events()) break;

        SDL_Keycode key = renderer.last_key();
        renderer.clear_last_key();
        if (key == SDLK_ESCAPE) break;
        if (key == SDLK_SPACE)  paused = !paused;
        if (key == SDLK_r) {
            std::fill(T.begin(), T.end(), 0.0);
            apply_bc(T);
            frame = 0; converged = false;
        }

        if (!paused && !converged) {
            double change = 0.0;
            for (int s = 0; s < spf; ++s) {
                for (int j = 1; j < ny-1; ++j) {
                    for (int i = 1; i < nx-1; ++i) {
                        double t = T[grid.idx(i,j)];
                        double d2x = T[grid.idx(i+1,j)] - 2*t + T[grid.idx(i-1,j)];
                        double d2y = T[grid.idx(i,j+1)] - 2*t + T[grid.idx(i,j-1)];
                        T_new[grid.idx(i,j)] = t + rx*d2x + ry*d2y;
                    }
                }
                apply_bc(T_new);
                change = 0.0;
                for (int k = 0; k < nx*ny; ++k)
                    change = std::max(change, std::abs(T_new[k] - T[k]));
                std::swap(T, T_new);
                ++frame;
                if (change < 1e-6) { converged = true; break; }
            }
        }

        renderer.render(zero, zero, zero, T, nx, ny, RenderMode::TEMPERATURE);

        char title[256];
        snprintf(title, sizeof(title),
            "Heat2D | adim=%d | Sol:SICAK Sag:SOGUK Ust/Alt:YALITILMIS%s%s",
            frame,
            converged ? " | YAKINSAMIS" : "",
            paused    ? " | [DURDURULDU]" : "");
        renderer.set_title(title);

        SDL_Delay(1);
    }
    return 0;
}
