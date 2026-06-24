#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// IRenderer — Abstract visualisation back-end
//
// Decouples the solver from SDL2 / OpenGL / headless / test renderers.
// ─────────────────────────────────────────────────────────────────────────────

#include <vector>
#include <string>

enum class RenderMode { VELOCITY, TEMPERATURE, PRESSURE };

class IRenderer {
public:
    virtual ~IRenderer() = default;

    // Draw one frame from the given field arrays.
    // All arrays are flat, row-major, size nx*ny.
    virtual void render(
        const std::vector<double>& u,
        const std::vector<double>& v,
        const std::vector<double>& p,
        const std::vector<double>& temp,
        int nx, int ny,
        RenderMode mode) = 0;

    // Update window title / overlay text.
    virtual void set_title(const std::string& title) = 0;

    // Returns false when the user closes the window.
    virtual bool is_open() const = 0;

    // Poll OS events; returns false when the window should close.
    virtual bool poll_events() = 0;
};
