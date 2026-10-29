#pragma once

#include <array>
#include <optional>
#include <string>
#include <vector>

namespace headunit {

// One calibration sample: where the raw touch was (normalised to 0..1 over the
// device's axis range) and where the target was (normalised over the screen).
struct CalibrationPoint {
    double rawX = 0;
    double rawY = 0;
    double screenX = 0;
    double screenY = 0;
};

// A libinput calibration matrix, row-major "a b c d e f":
//   screenX = a * rawX + b * rawY + c
//   screenY = d * rawX + e * rawY + f
using CalibrationMatrix = std::array<double, 6>;

struct CalibrationResult {
    CalibrationMatrix matrix{};
    // Root-mean-square distance between each target and its mapped tap, in pixels.
    double rmsPixels = 0;
    double maxPixels = 0;
};

// Least-squares affine fit. Needs at least three points that are not collinear;
// returns nothing for degenerate or non-finite input.
std::optional<CalibrationResult> solveCalibration(const std::vector<CalibrationPoint>& points, double screenWidth,
                                                  double screenHeight);

// "a b c d e f" with enough digits for libinput, no trailing zeros.
std::string formatMatrix(const CalibrationMatrix& matrix);

// Parses six finite numbers separated by whitespace; nothing else is accepted.
std::optional<CalibrationMatrix> parseMatrix(const std::string& text);

// Median of a press's samples, ignoring the first and last few (a resistive
// panel is noisy while the finger lands and lifts). Empty input gives nothing.
std::optional<double> stableMedian(std::vector<double> samples);

}  // namespace headunit
