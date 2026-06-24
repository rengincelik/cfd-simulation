#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// Colormap — maps a normalised scalar t ∈ [0,1] to an RGB colour
//
// Available maps: BLUE_RED (cool→hot), GRAYSCALE, DIVERGING (blue-white-red)
// ─────────────────────────────────────────────────────────────────────────────

#include <cstdint>
#include <algorithm>
#include <cmath>

struct RGB { std::uint8_t r, g, b; };

class Colormap {
public:
    enum class Map { BLUE_RED, GRAYSCALE, DIVERGING };

    explicit Colormap(Map m = Map::BLUE_RED) : map_(m) {}

    // t must be in [0,1]; clamped internally.
    RGB operator()(double t) const {
        t = std::max(0.0, std::min(1.0, t));
        switch (map_) {
            case Map::GRAYSCALE: return grayscale(t);
            case Map::DIVERGING: return diverging(t);
            default:             return blue_red(t);
        }
    }

    // Pack as ARGB uint32 for SDL2 textures (alpha = 255).
    std::uint32_t argb(double t) const {
        auto [r, g, b] = (*this)(t);
        return (255u << 24) | ((std::uint32_t)r << 16)
                            | ((std::uint32_t)g <<  8)
                            | b;
    }

private:
    Map map_;

    // Blue → Cyan → Green → Yellow → Red
    static RGB blue_red(double t) {
        double r, g, b;
        if      (t < 0.25) { double s=t/0.25;       r=0;  g=s;  b=1;   }
        else if (t < 0.50) { double s=(t-0.25)/0.25; r=0;  g=1;  b=1-s; }
        else if (t < 0.75) { double s=(t-0.50)/0.25; r=s;  g=1;  b=0;   }
        else               { double s=(t-0.75)/0.25; r=1;  g=1-s;b=0;   }
        return { (std::uint8_t)(r*255), (std::uint8_t)(g*255), (std::uint8_t)(b*255) };
    }

    static RGB grayscale(double t) {
        auto v = (std::uint8_t)(t * 255);
        return {v, v, v};
    }

    // Blue(-1) → White(0) → Red(+1)   — useful for signed pressure
    static RGB diverging(double t) {
        double r, g, b;
        if (t < 0.5) { double s = t/0.5; r=s; g=s; b=1; }
        else         { double s = (t-0.5)/0.5; r=1; g=1-s; b=1-s; }
        return { (std::uint8_t)(r*255), (std::uint8_t)(g*255), (std::uint8_t)(b*255) };
    }
};
