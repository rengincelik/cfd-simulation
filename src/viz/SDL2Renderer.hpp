#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// SDL2Renderer — concrete IRenderer backed by SDL2
//
// Renders velocity magnitude, temperature, or pressure as a colour map.
// In VELOCITY mode: overlays velocity arrows every `arrow_step` grid cells.
// ─────────────────────────────────────────────────────────────────────────────

#include "../../include/IRenderer.hpp"
#include "Colormap.hpp"

#include <SDL2/SDL.h>
#include <string>
#include <vector>
#include <cmath>
#include <cstdio>
#include <stdexcept>

class SDL2Renderer : public IRenderer {
public:
    // win_size  — pixel size of the square window
    // arrow_step — draw a velocity arrow every N grid cells (0 = no arrows)
    SDL2Renderer(int nx, int ny,
                 int win_size  = 640,
                 int arrow_step = 4)
        : nx_(nx), ny_(ny)
        , win_size_(win_size)
        , arrow_step_(arrow_step)
        , open_(true)
        , cmap_vel_(Colormap::Map::BLUE_RED)
        , cmap_tmp_(Colormap::Map::BLUE_RED)
        , cmap_prs_(Colormap::Map::DIVERGING)
    {
        if (SDL_Init(SDL_INIT_VIDEO) != 0)
            throw std::runtime_error(std::string("SDL_Init: ") + SDL_GetError());

        win_ = SDL_CreateWindow("CFD Solver",
            SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
            win_size_, win_size_, 0);
        ren_ = SDL_CreateRenderer(win_, -1, SDL_RENDERER_ACCELERATED);
        tex_ = SDL_CreateTexture(ren_,
            SDL_PIXELFORMAT_ARGB8888,
            SDL_TEXTUREACCESS_STREAMING,
            win_size_, win_size_);
    }

    ~SDL2Renderer() override {
        SDL_DestroyTexture(tex_);
        SDL_DestroyRenderer(ren_);
        SDL_DestroyWindow(win_);
        SDL_Quit();
    }

    // ── IRenderer ────────────────────────────────────────────────────────────

    void render(const std::vector<double>& u,
                const std::vector<double>& v,
                const std::vector<double>& p,
                const std::vector<double>& temp,
                int nx, int ny,
                RenderMode mode) override
    {
        // Compute normalisation range
        double vmax = 1e-9, pmin = 1e9, pmax = -1e9;
        for (int k = 0; k < nx*ny; ++k) {
            double spd = std::sqrt(u[k]*u[k] + v[k]*v[k]);
            if (spd > vmax) vmax = spd;
            if (p[k] < pmin) pmin = p[k];
            if (p[k] > pmax) pmax = p[k];
        }

        int cell = win_size_ / nx;
        pixels_.assign(win_size_ * win_size_, 0u);

        auto idx = [&](int i, int j){ return j * nx + i; };

        for (int j = 0; j < ny; ++j) {
            for (int i = 0; i < nx; ++i) {
                double t = 0.0;
                if (mode == RenderMode::VELOCITY) {
                    double spd = std::sqrt(u[idx(i,j)]*u[idx(i,j)] +
                                           v[idx(i,j)]*v[idx(i,j)]);
                    t = spd / vmax;
                    fill_cell(pixels_, i, j, ny, cell, cmap_vel_.argb(t));
                } else if (mode == RenderMode::TEMPERATURE) {
                    fill_cell(pixels_, i, j, ny, cell, cmap_tmp_.argb(temp[idx(i,j)]));
                } else {
                    t = (pmax > pmin) ? (p[idx(i,j)] - pmin) / (pmax - pmin) : 0.5;
                    fill_cell(pixels_, i, j, ny, cell, cmap_prs_.argb(t));
                }
            }
        }

        SDL_UpdateTexture(tex_, nullptr, pixels_.data(), win_size_ * sizeof(std::uint32_t));
        SDL_RenderCopy(ren_, tex_, nullptr, nullptr);

        // Velocity arrows
        if (mode == RenderMode::VELOCITY && arrow_step_ > 0) {
            SDL_SetRenderDrawColor(ren_, 255, 255, 255, 180);
            for (int j = arrow_step_/2; j < ny; j += arrow_step_) {
                for (int i = arrow_step_/2; i < nx; i += arrow_step_) {
                    double uc = u[idx(i,j)];
                    double vc = v[idx(i,j)];
                    double spd = std::sqrt(uc*uc + vc*vc);
                    if (spd < 1e-6) continue;
                    int cx = (int)((i + 0.5) * cell);
                    int cy = (int)((ny - 1 - j + 0.5) * cell);
                    double scale = (cell * arrow_step_ * 0.4) / vmax;
                    int ex = cx + (int)(uc * scale);
                    int ey = cy - (int)(vc * scale);
                    SDL_RenderDrawLine(ren_, cx, cy, ex, ey);
                }
            }
        }

        SDL_RenderPresent(ren_);
    }

    void set_title(const std::string& title) override {
        SDL_SetWindowTitle(win_, title.c_str());
    }

    bool is_open() const override { return open_; }

    bool poll_events() override {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) { open_ = false; return false; }
            if (e.type == SDL_KEYDOWN) {
                if (e.key.keysym.sym == SDLK_ESCAPE) { open_ = false; return false; }
                // Forward key to caller via stored last key
                last_key_ = e.key.keysym.sym;
            }
        }
        return open_;
    }

    // Last key pressed since previous poll_events() call.
    SDL_Keycode last_key() const { return last_key_; }
    void clear_last_key() { last_key_ = SDLK_UNKNOWN; }

private:
    int  nx_, ny_, win_size_, arrow_step_;
    bool open_;

    SDL_Window*   win_ = nullptr;
    SDL_Renderer* ren_ = nullptr;
    SDL_Texture*  tex_ = nullptr;

    Colormap cmap_vel_, cmap_tmp_, cmap_prs_;
    std::vector<std::uint32_t> pixels_;
    SDL_Keycode last_key_ = SDLK_UNKNOWN;

    void fill_cell(std::vector<std::uint32_t>& buf,
                   int i, int j, int ny,
                   int cell,
                   std::uint32_t color) const
    {
        int py = (ny - 1 - j) * cell;
        int px = i * cell;
        for (int dy = 0; dy < cell; ++dy)
            for (int dx = 0; dx < cell; ++dx)
                buf[(py+dy) * win_size_ + (px+dx)] = color;
    }
};
