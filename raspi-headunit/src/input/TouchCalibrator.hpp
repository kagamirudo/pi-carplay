#pragma once

#include "input/TouchCalibration.hpp"

#include <QObject>
#include <QPointF>
#include <QString>
#include <QTimer>
#include <memory>
#include <vector>

class QSocketNotifier;

namespace headunit {

// Drives `headunit --calibrate-touch`: shows one target at a time and records
// where the finger really was, read straight from the evdev node so the
// compositor's current calibration does not bias the result.
class TouchCalibrator : public QObject {
    Q_OBJECT
    Q_PROPERTY(int step READ step NOTIFY changed)
    Q_PROPERTY(int steps READ steps CONSTANT)
    Q_PROPERTY(QPointF target READ target NOTIFY changed)
    Q_PROPERTY(QString message READ message NOTIFY changed)
    Q_PROPERTY(int secondsLeft READ secondsLeft NOTIFY tick)
    Q_PROPERTY(bool finished READ finished NOTIFY changed)

public:
    enum Outcome { Pending = -1, Done = 0, NoDevice = 2, TimedOut = 3, Inconsistent = 4 };
    Q_ENUM(Outcome)

    TouchCalibrator(double screenWidth, double screenHeight, int timeoutSeconds, QObject* parent = nullptr);
    ~TouchCalibrator() override;

    int step() const { return step_; }
    int steps() const { return static_cast<int>(targets_.size()); }
    QPointF target() const;
    QString message() const { return message_; }
    int secondsLeft() const { return secondsLeft_; }
    bool finished() const { return outcome_ != Pending; }
    Outcome outcome() const { return outcome_; }
    const CalibrationResult& result() const { return result_; }
    const std::vector<CalibrationPoint>& points() const { return points_; }

    // Reads from an evdev node (EVIOCGABS gives the axis ranges).
    bool open(const QString& device, QString* error);
    // Axis ranges and events, for tests and for open().
    void setAxisRange(int minX, int maxX, int minY, int maxY);
    void feedEvent(int type, int code, int value);
    // Taps further than this from the fitted line fail the calibration.
    static constexpr double kMaxErrorPixels = 20.0;

signals:
    void changed();
    void tick();
    void completed(int outcome);

private:
    void readDevice();
    void release();
    void advance(double rawX, double rawY);
    void finish(Outcome outcome, const QString& message);
    void restartTimer();

    double width_;
    double height_;
    int timeoutSeconds_;
    std::vector<QPointF> targets_;
    std::vector<CalibrationPoint> points_;
    int step_ = 0;
    int secondsLeft_ = 0;
    QString message_;
    Outcome outcome_ = Pending;
    CalibrationResult result_;
    QTimer timer_;

    int fd_ = -1;
    std::unique_ptr<QSocketNotifier> notifier_;
    int minX_ = 0, maxX_ = 4095, minY_ = 0, maxY_ = 4095;
    int x_ = 0, y_ = 0;
    bool down_ = false;
    std::vector<double> xs_, ys_;
};

}  // namespace headunit
