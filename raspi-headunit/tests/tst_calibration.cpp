#include "input/TouchCalibration.hpp"

#include <QTest>

#include <cmath>
#include <optional>
#include <vector>

using headunit::CalibrationMatrix;
using headunit::CalibrationPoint;
using headunit::formatMatrix;
using headunit::parseMatrix;
using headunit::solveCalibration;
using headunit::stableMedian;

namespace {

// Build sample points from a known affine map (normalised raw -> normalised screen).
std::vector<CalibrationPoint> pointsFrom(const CalibrationMatrix& m) {
    const double raw[5][2] = {{0.1, 0.1}, {0.9, 0.1}, {0.9, 0.9}, {0.1, 0.9}, {0.5, 0.5}};
    std::vector<CalibrationPoint> pts;
    for (auto& r : raw) {
        pts.push_back({r[0], r[1], m[0] * r[0] + m[1] * r[1] + m[2], m[3] * r[0] + m[4] * r[1] + m[5]});
    }
    return pts;
}

}  // namespace

class CalibrationTest : public QObject {
    Q_OBJECT

private slots:
    void recoversKnownMatrix() {
        const CalibrationMatrix known{1.05, 0.0, -0.02, 0.0, 0.98, 0.03};
        const auto r = solveCalibration(pointsFrom(known), 480, 320);
        QVERIFY(r.has_value());
        for (int i = 0; i < 6; ++i) {
            QVERIFY(std::fabs(r->matrix[i] - known[i]) < 1e-9);
        }
        QVERIFY(r->rmsPixels < 1e-6);
        QVERIFY(r->maxPixels < 1e-6);
    }

    void reportsErrorInPixels() {
        auto pts = pointsFrom({1, 0, 0, 0, 1, 0});  // identity
        pts[4].screenX += 10.0 / 480.0;             // nudge the centre tap 10 px in x
        const auto r = solveCalibration(pts, 480, 320);
        QVERIFY(r.has_value());
        QVERIFY(r->maxPixels > 1.0);  // the fit cannot hide a real offset
    }

    void rejectsTooFewPoints() {
        std::vector<CalibrationPoint> two{{0, 0, 0, 0}, {1, 1, 1, 1}};
        QVERIFY(!solveCalibration(two, 480, 320).has_value());
    }

    void rejectsCollinearPoints() {
        std::vector<CalibrationPoint> line{{0.1, 0.1, 0.1, 0.1}, {0.5, 0.5, 0.5, 0.5}, {0.9, 0.9, 0.9, 0.9}};
        QVERIFY(!solveCalibration(line, 480, 320).has_value());
    }

    void rejectsBadScreenSize() {
        QVERIFY(!solveCalibration(pointsFrom({1, 0, 0, 0, 1, 0}), 0, 320).has_value());
    }

    void formatsWithoutTrailingZerosOrNegativeZero() {
        QCOMPARE(formatMatrix({1, 0, 0, 0, -1, 1}), std::string("1 0 0 0 -1 1"));
        QCOMPARE(formatMatrix({1.5, -0.0, 0.25, 0, 0, 0}), std::string("1.5 0 0.25 0 0 0"));
    }

    void formatParseRoundTrip() {
        const CalibrationMatrix m{1.050001, -0.0, -0.02, 0.123456, 0.98, 0.030003};
        const auto back = parseMatrix(formatMatrix(m));
        QVERIFY(back.has_value());
        for (int i = 0; i < 6; ++i) {
            QVERIFY(std::fabs((*back)[i] - m[i]) < 1e-6);
        }
    }

    void parseRejectsMalformed() {
        QVERIFY(!parseMatrix("1 2 3").has_value());            // too few
        QVERIFY(!parseMatrix("1 2 3 4 5 6 7").has_value());    // too many
        QVERIFY(!parseMatrix("1 2 3 4 5 x").has_value());      // not a number
        QVERIFY(!parseMatrix("1 2 3 4 5 6extra").has_value()); // trailing junk on a token
        QVERIFY(parseMatrix("  1 0 0 0 -1 1 ").has_value());   // surrounding space is fine
    }

    void stableMedianBasics() {
        QVERIFY(!stableMedian({}).has_value());
        QCOMPARE(*stableMedian({3, 1, 2}), 2.0);
        // >= 8 samples: the two extremes at each end are trimmed before the median.
        QCOMPARE(*stableMedian({999, 999, 5, 5, 5, 5, -999, -999}), 5.0);
    }
};

QTEST_MAIN(CalibrationTest)
#include "tst_calibration.moc"
