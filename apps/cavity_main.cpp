// ─────────────────────────────────────────────────────────────────────────────
// cavity_main.cpp — Lid-Driven Cavity CFD application
//
// Reads config from argv[1] (defaults to "cavity.cfg" if omitted).
//
// Keys:
//   1  → velocity view
//   2  → temperature view
//   3  → pressure view
//   Space → pause / resume
//   V     → run Ghia validation and print report
//   R     → reset simulation
//   Esc   → quit
//
// Build:
//   cmake -B build && cmake --build
//   ./build/cavity          (uses cavity.cfg)
//   ./build/cavity re1000.cfg
// ─────────────────────────────────────────────────────────────────────────────

#include "../include/Config.hpp"
#include "../src/engine/NavierStokesSolver.hpp"
#include "../src/validation/GhiaValidator.hpp"
#include "../src/viz/SDL2Renderer.hpp"
#include "../src/viz/CSVExporter.hpp"

#include <iostream>
#include <memory>
#include <string>
#include <cstdio>



int main(int argc, char* argv[]) {
    const char* cfg_path = (argc > 1) ? argv[1] : "cavity.cfg";
    Config cfg = Config::from_file(cfg_path);


    int    nx          = cfg.get_int   ("nx",           64);
    int    ny          = cfg.get_int   ("ny",           64);
    int    win_size    = cfg.get_int   ("win_size",    640);
    int    arrow_step  = cfg.get_int   ("arrow_step",    4);
    int    export_every= cfg.get_int   ("export_every",  0);
    double re          = cfg.get_double("reynolds",   400.0);

    std::cout << "── Lid-Driven Cavity CFD ─────────────────────────\n";
    std::cout << "  grid     : " << nx << " x " << ny << "\n";
    std::cout << "  Re       : " << re << "\n";
    std::cout << "  dt       : " << cfg.get_double("dt", 0.001) << "\n";
    std::cout << "  p_iter   : " << cfg.get_int("p_iter", 50) << "\n";
    std::cout << "─────────────────────────────────────────────────\n\n";

    // ── Solver ───────────────────────────────────────────────────────────────
    NavierStokesSolver solver(cfg);

    // ── Renderer ─────────────────────────────────────────────────────────────
    SDL2Renderer renderer(nx, ny, win_size, arrow_step);

    // ── Exporter (optional) ──────────────────────────────────────────────────
    CSVExporter exporter("output/cavity");

    // ── Validator (only for Re with reference data) ───────────────────────────
    std::unique_ptr<GhiaValidator> validator;
    int re_int = (int)re;
    if (re_int == 100 || re_int == 400 || re_int == 1000) {
        validator = std::make_unique<GhiaValidator>(re_int);
    }

    RenderMode mode  = RenderMode::VELOCITY;
    bool       paused = false;

    // ── Main loop ─────────────────────────────────────────────────────────────
    while (renderer.is_open()) {
        // Events
        if (!renderer.poll_events()) break;

        SDL_Keycode key = renderer.last_key();
        renderer.clear_last_key();

        if (key == SDLK_ESCAPE) break;
        if (key == SDLK_SPACE)  paused = !paused;
        if (key == SDLK_1)      mode   = RenderMode::VELOCITY;
        if (key == SDLK_2)      mode   = RenderMode::TEMPERATURE;
        if (key == SDLK_3)      mode   = RenderMode::PRESSURE;
        if (key == SDLK_r)    { solver = NavierStokesSolver(cfg); }
        if (key == SDLK_v && validator) {
            validator->print_report(solver);
        }

        // Advance simulation
        if (!paused) {
            solver.step();

            // Export to CSV if requested
            if (export_every > 0 && solver.step_count() % export_every == 0) {
                const IGrid& g = solver.grid();
                exporter.write_all(g,
                    solver.field_u(), solver.field_v(),
                    solver.field_p(), solver.field_temp(),
                    solver.step_count());
            }
        }

        // Render every 5 steps (keep UI responsive)
        if (solver.step_count() % 5 == 0 || paused) {
            renderer.render(
                solver.field_u(), solver.field_v(),
                solver.field_p(), solver.field_temp(),
                nx, ny, mode);

            char title[256];
            std::snprintf(title, sizeof(title),
                "Lid-Driven Cavity | Re=%.0f | step=%d | t=%.3f | %s%s",
                re, solver.step_count(), solver.sim_time(),
                mode == RenderMode::VELOCITY    ? "VELOCITY" :
                mode == RenderMode::TEMPERATURE ? "TEMPERATURE" : "PRESSURE",
                paused ? " [PAUSED]" : "");
            renderer.set_title(title);
        }

        SDL_Delay(1);
    }

    // Final validation report
    if (validator) {
        std::cout << "\n── Final validation ──\n";
        validator->print_report(solver);
    }

    return 0;
}
