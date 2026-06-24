#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// CSVExporter — writes field arrays to space-separated CSV files
//
// Output format: one row per grid row (j=0 first), space-separated doubles.
// Load in Python: np.loadtxt("u_step_01000.csv")
// ─────────────────────────────────────────────────────────────────────────────

#include "../../include/IGrid.hpp"
#include <vector>
#include <string>
#include <fstream>
#include <iomanip>
#include <stdexcept>

class CSVExporter {
public:
    // prefix — prepended to every filename, e.g. "output/cavity"
    explicit CSVExporter(std::string prefix = "field") : prefix_(std::move(prefix)) {}

    // Write one field.  filename: "<prefix>_<name>_<step>.csv"
    void write(const IGrid& g,
               const std::vector<double>& field,
               const std::string& name,
               int step) const
    {
        std::string path = prefix_ + "_" + name + "_"
                         + zero_pad(step, 6) + ".csv";
        std::ofstream f(path);
        if (!f.is_open())
            throw std::runtime_error("CSVExporter: cannot write '" + path + "'");

        f << std::scientific << std::setprecision(8);
        for (int j = 0; j < g.ny(); ++j) {
            for (int i = 0; i < g.nx(); ++i) {
                f << field[g.idx(i, j)];
                if (i < g.nx()-1) f << ' ';
            }
            f << '\n';
        }
    }

    // Convenience: write u, v, p, temp in one call.
    void write_all(const IGrid& g,
                   const std::vector<double>& u,
                   const std::vector<double>& v,
                   const std::vector<double>& p,
                   const std::vector<double>& temp,
                   int step) const
    {
        write(g, u,    "u",    step);
        write(g, v,    "v",    step);
        write(g, p,    "p",    step);
        write(g, temp, "temp", step);
    }

private:
    std::string prefix_;

    static std::string zero_pad(int n, int width) {
        std::string s = std::to_string(n);
        while ((int)s.size() < width) s = "0" + s;
        return s;
    }
};
