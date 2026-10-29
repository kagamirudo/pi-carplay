#include "input/TouchCalibration.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <sstream>

namespace headunit {

namespace {

// Solves the 3x3 system m * x = v by Cramer's rule.
std::optional<std::array<double, 3>> solve3(const std::array<std::array<double, 3>, 3>& m,
                                            const std::array<double, 3>& v) {
    const auto det = [](const std::array<std::array<double, 3>, 3>& a) {
        return a[0][0] * (a[1][1] * a[2][2] - a[1][2] * a[2][1]) - a[0][1] * (a[1][0] * a[2][2] - a[1][2] * a[2][0]) +
               a[0][2] * (a[1][0] * a[2][1] - a[1][1] * a[2][0]);
    };
    const double d = det(m);
    if (!std::isfinite(d) || std::fabs(d) < 1e-12) {
        return std::nullopt;
    }
    std::array<double, 3> out{};
    for (int col = 0; col < 3; ++col) {
        auto replaced = m;
        for (int row = 0; row < 3; ++row) {
            replaced[row][col] = v[row];
        }
        out[col] = det(replaced) / d;
    }
    return out;
}

}  // namespace

std::optional<CalibrationResult> solveCalibration(const std::vector<CalibrationPoint>& points, double screenWidth,
                                                  double screenHeight) {
    if (points.size() < 3 || !(screenWidth > 0) || !(screenHeight > 0)) {
        return std::nullopt;
    }
    // Normal equations: (A^T A) m = A^T b, with rows of A = [rawX rawY 1].
    std::array<std::array<double, 3>, 3> ata{};
    std::array<double, 3> atx{};
    std::array<double, 3> aty{};
    for (const auto& p : points) {
        const std::array<double, 3> row{p.rawX, p.rawY, 1.0};
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                ata[i][j] += row[i] * row[j];
            }
            atx[i] += row[i] * p.screenX;
            aty[i] += row[i] * p.screenY;
        }
    }
    const auto xRow = solve3(ata, atx);
    const auto yRow = solve3(ata, aty);
    if (!xRow || !yRow) {
        return std::nullopt;
    }

    CalibrationResult result;
    result.matrix = {(*xRow)[0], (*xRow)[1], (*xRow)[2], (*yRow)[0], (*yRow)[1], (*yRow)[2]};
    if (!std::all_of(result.matrix.begin(), result.matrix.end(), [](double v) { return std::isfinite(v); })) {
        return std::nullopt;
    }
    double sum = 0;
    for (const auto& p : points) {
        const auto& m = result.matrix;
        const double dx = (m[0] * p.rawX + m[1] * p.rawY + m[2] - p.screenX) * screenWidth;
        const double dy = (m[3] * p.rawX + m[4] * p.rawY + m[5] - p.screenY) * screenHeight;
        const double d2 = dx * dx + dy * dy;
        sum += d2;
        result.maxPixels = std::max(result.maxPixels, std::sqrt(d2));
    }
    result.rmsPixels = std::sqrt(sum / static_cast<double>(points.size()));
    return result;
}

std::string formatMatrix(const CalibrationMatrix& matrix) {
    std::string out;
    for (size_t i = 0; i < matrix.size(); ++i) {
        char buf[32];
        double v = std::round(matrix[i] * 1e6) / 1e6;
        if (v == 0) {
            v = 0;  // no "-0"
        }
        std::snprintf(buf, sizeof buf, "%.6f", v);
        std::string s(buf);
        s.erase(s.find_last_not_of('0') + 1);
        if (s.back() == '.') {
            s.pop_back();
        }
        out += (i ? " " : "") + s;
    }
    return out;
}

std::optional<CalibrationMatrix> parseMatrix(const std::string& text) {
    std::istringstream in(text);
    CalibrationMatrix m{};
    for (auto& v : m) {
        std::string token;
        if (!(in >> token)) {
            return std::nullopt;
        }
        size_t used = 0;
        try {
            v = std::stod(token, &used);
        } catch (...) {
            return std::nullopt;
        }
        if (used != token.size() || !std::isfinite(v)) {
            return std::nullopt;
        }
    }
    std::string extra;
    if (in >> extra) {
        return std::nullopt;
    }
    return m;
}

std::optional<double> stableMedian(std::vector<double> samples) {
    if (samples.empty()) {
        return std::nullopt;
    }
    const size_t trim = samples.size() >= 8 ? 2 : 0;
    std::vector<double> kept(samples.begin() + static_cast<long>(trim), samples.end() - static_cast<long>(trim));
    std::sort(kept.begin(), kept.end());
    const size_t n = kept.size();
    return n % 2 ? kept[n / 2] : (kept[n / 2 - 1] + kept[n / 2]) / 2;
}

}  // namespace headunit
