#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// Config — plain text key-value config file parser
//
// Format:  key value   (one per line, # comments stripped)
//
// Usage:
//   Config cfg = Config::from_file("cavity.cfg");
//   double re  = cfg.get_double("reynolds", 400.0);
// ─────────────────────────────────────────────────────────────────────────────

#include <string>
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <stdexcept>

class Config {
public:
    // Parse from file; throws std::runtime_error on missing file.
    static Config from_file(const std::string& path) {
        std::ifstream f(path);
        if (!f.is_open())
            throw std::runtime_error("Config: cannot open '" + path + "'");
        Config cfg;
        std::string line;
        while (std::getline(f, line)) {
            auto c = line.find('#');
            if (c != std::string::npos) line = line.substr(0, c);
            std::istringstream iss(line);
            std::string key;
            if (!(iss >> key)) continue;
            std::string val;
            iss >> val;
            cfg.data_[key] = val;
        }
        return cfg;
    }

    // Parse from string (useful for tests / defaults).
    static Config from_string(const std::string& text) {
        Config cfg;
        std::istringstream ss(text);
        std::string line;
        while (std::getline(ss, line)) {
            auto c = line.find('#');
            if (c != std::string::npos) line = line.substr(0, c);
            std::istringstream iss(line);
            std::string key;
            if (!(iss >> key)) continue;
            std::string val;
            iss >> val;
            cfg.data_[key] = val;
        }
        return cfg;
    }

    // Getters with defaults.
    double      get_double(const std::string& k, double      def = 0.0)   const { return has(k) ? std::stod(data_.at(k))   : def; }
    int         get_int   (const std::string& k, int         def = 0)     const { return has(k) ? std::stoi(data_.at(k))   : def; }
    std::string get_string(const std::string& k, std::string def = "")    const { return has(k) ? data_.at(k)              : def; }
    bool        get_bool  (const std::string& k, bool        def = false) const {
        if (!has(k)) return def;
        const auto& v = data_.at(k);
        return (v == "true" || v == "1" || v == "yes");
    }

    bool has(const std::string& k) const { return data_.count(k) > 0; }

private:
    std::unordered_map<std::string, std::string> data_;
};
