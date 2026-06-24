// ─────────────────────────────────────────────────────────────────────────────
// heat2d_obstacle_main.cpp — 2D Heat Diffusion + yalıtılmış engel
//
// Disk içi hesaptan dışlanır (Neumann ∂T/∂n=0), ısı akışı geçemez.
// Sol tık + sürükle → diski hareket ettir
//
// Tuşlar: Space durdur/devam, R sıfırla, Esc çık
// ─────────────────────────────────────────────────────────────────────────────

#include "../include/Config.hpp"
#include "../src/engine/UniformGrid.hpp"
#include "../src/engine/bc/DirichletBC.hpp"
#include "../src/engine/bc/NeumannBC.hpp"
#include "../src/viz/SDL2Renderer.hpp"

#include <SDL2/SDL.h>
#include <vector>
#include <algorithm>
#include <cmath>
#include <cstdio>



struct Disk {
    double cx, cy, r;
    bool dragging = false;
    bool contains(int gi, int gj) const {
        double dx = gj - cx, dy = gi - cy;
        return dx*dx + dy*dy <= r*r;
    }
};

int main(int argc, char* argv[]) {
    const char* cfg_path = (argc > 1) ? argv[1] : "heat2d_obstacle.cfg";
    Config cfg = Config::from_file(cfg_path);


    int    nx    = cfg.get_int   ("nx",       128);
    int    ny    = cfg.get_int   ("ny",       128);
    double alpha = cfg.get_double("alpha",   0.01);
    int    win   = cfg.get_int   ("win_size", 640);
    int    spf   = cfg.get_int   ("steps_per_frame", 20);

    UniformGrid grid(nx, ny);
    double dx2 = grid.dx() * grid.dx();
    double dy2 = grid.dy() * grid.dy();
    double dt  = 0.24 * std::min(dx2, dy2) / alpha;
    double rx  = alpha * dt / dx2;
    double ry  = alpha * dt / dy2;

    Disk disk{ (double)nx/2, (double)ny/2, (double)ny/10 };

    DirichletBC left_hot  (DirichletBC::LEFT,  1.0);
    DirichletBC right_cold(DirichletBC::RIGHT, 0.0);
    NeumannBC   insulated (NeumannBC::BOTTOM | NeumannBC::TOP);

    auto apply_bc = [&](std::vector<double>& T) {
        left_hot.apply(grid, T);
        right_cold.apply(grid, T);
        insulated.apply(grid, T);
    };

    std::vector<double> T    (nx*ny, 0.0);
    std::vector<double> T_new(nx*ny, 0.0);
    apply_bc(T);

    SDL2Renderer renderer(nx, ny, win, 0);
    std::vector<double> zero(nx*ny, 0.0);

    bool paused    = false;
    bool converged = false;
    int  frame     = 0;

    while (renderer.is_open()) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) goto done;
            if (e.type == SDL_KEYDOWN) {
                if (e.key.keysym.sym == SDLK_ESCAPE) goto done;
                if (e.key.keysym.sym == SDLK_SPACE) paused = !paused;
                if (e.key.keysym.sym == SDLK_r) {
                    std::fill(T.begin(), T.end(), 0.0);
                    apply_bc(T); frame = 0; converged = false;
                }
            }
            if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
                int gj = e.button.x * nx / win;
                int gi = (ny-1) - (e.button.y * ny / win);
                if (disk.contains(gi, gj)) disk.dragging = true;
            }
            if (e.type == SDL_MOUSEBUTTONUP) disk.dragging = false;
            if (e.type == SDL_MOUSEMOTION && disk.dragging) {
                double new_cx = (double)e.motion.x * nx / win;
                double new_cy = (double)(win-1-e.motion.y) * ny / win;
                new_cx = std::max(disk.r+1, std::min((double)nx-1-disk.r-1, new_cx));
                new_cy = std::max(disk.r+1, std::min((double)ny-1-disk.r-1, new_cy));
                disk.cx = new_cx; disk.cy = new_cy;
                converged = false;
            }
        }

        if (!paused && !converged) {
            double change = 0.0;
            for (int s = 0; s < spf; ++s) {
                for (int j = 1; j < ny-1; ++j) {
                    for (int i = 1; i < nx-1; ++i) {
                        // Disk içindeyse güncelleme
                        if (disk.contains(j, i)) {
                            T_new[grid.idx(i,j)] = T[grid.idx(i,j)];
                            continue;
                        }
                        // Komşu disk içindeyse ghost cell = kendisi (Neumann)
                        double Txm = disk.contains(j,   i-1) ? T[grid.idx(i,j)] : T[grid.idx(i-1,j)];
                        double Txp = disk.contains(j,   i+1) ? T[grid.idx(i,j)] : T[grid.idx(i+1,j)];
                        double Tym = disk.contains(j-1, i  ) ? T[grid.idx(i,j)] : T[grid.idx(i,j-1)];
                        double Typ = disk.contains(j+1, i  ) ? T[grid.idx(i,j)] : T[grid.idx(i,j+1)];

                        double t   = T[grid.idx(i,j)];
                        double d2x = Txm - 2*t + Txp;
                        double d2y = Tym - 2*t + Typ;
                        T_new[grid.idx(i,j)] = t + rx*d2x + ry*d2y;
                    }
                }
                apply_bc(T_new);
                change = 0.0;
                for (int k = 0; k < nx*ny; ++k)
                    change = std::max(change, std::abs(T_new[k]-T[k]));
                std::swap(T, T_new);
                ++frame;
                if (change < 1e-6) { converged = true; break; }
            }
        }

        renderer.render(zero, zero, zero, T, nx, ny, RenderMode::TEMPERATURE);

        char title[256];
        snprintf(title, sizeof(title),
            "Heat2D+Engel | adim=%d%s%s", frame,
            converged ? " | YAKINSAMIS" : "",
            paused    ? " | [DURDURULDU]" : "");
        renderer.set_title(title);
        SDL_Delay(1);
    }
    done:
    return 0;
}
