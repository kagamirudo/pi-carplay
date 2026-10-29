#include "input/TouchCalibrator.hpp"

#include <QSocketNotifier>

#include <fcntl.h>
#include <linux/input.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>

namespace headunit {

namespace {

// Corners inset by 10% plus the centre: wide enough to measure scale, far
// enough from the bezel that a fingertip can hit them.
std::vector<QPointF> targetsFor(double w, double h) {
    const double mx = w * 0.1;
    const double my = h * 0.1;
    return {{mx, my}, {w - mx, my}, {w - mx, h - my}, {mx, h - my}, {w / 2, h / 2}};
}

}  // namespace

TouchCalibrator::TouchCalibrator(double screenWidth, double screenHeight, int timeoutSeconds, QObject* parent)
    : QObject(parent),
      width_(screenWidth),
      height_(screenHeight),
      timeoutSeconds_(timeoutSeconds),
      targets_(targetsFor(screenWidth, screenHeight)),
      message_(QStringLiteral("Tap the centre of the cross")) {
    timer_.setInterval(1000);
    connect(&timer_, &QTimer::timeout, this, [this] {
        if (--secondsLeft_ <= 0) {
            finish(TimedOut, QStringLiteral("No tap in time. Calibration unchanged."));
        }
        emit tick();
    });
    restartTimer();
}

TouchCalibrator::~TouchCalibrator() {
    notifier_.reset();
    if (fd_ >= 0) {
        ::close(fd_);
    }
}

QPointF TouchCalibrator::target() const {
    return step_ < steps() ? targets_[static_cast<size_t>(step_)] : QPointF(width_ / 2, height_ / 2);
}

bool TouchCalibrator::open(const QString& device, QString* error) {
    fd_ = ::open(device.toLocal8Bit().constData(), O_RDONLY | O_NONBLOCK | O_CLOEXEC);
    if (fd_ < 0) {
        if (error) {
            *error = device + QStringLiteral(": ") + QString::fromLocal8Bit(std::strerror(errno));
        }
        return false;
    }
    input_absinfo ax{};
    input_absinfo ay{};
    if (::ioctl(fd_, EVIOCGABS(ABS_X), &ax) < 0 || ::ioctl(fd_, EVIOCGABS(ABS_Y), &ay) < 0) {
        if (error) {
            *error = device + QStringLiteral(": not an absolute touch device");
        }
        ::close(fd_);
        fd_ = -1;
        return false;
    }
    setAxisRange(ax.minimum, ax.maximum, ay.minimum, ay.maximum);
    notifier_ = std::make_unique<QSocketNotifier>(fd_, QSocketNotifier::Read);
    connect(notifier_.get(), &QSocketNotifier::activated, this, &TouchCalibrator::readDevice);
    return true;
}

void TouchCalibrator::setAxisRange(int minX, int maxX, int minY, int maxY) {
    minX_ = minX;
    maxX_ = maxX > minX ? maxX : minX + 1;
    minY_ = minY;
    maxY_ = maxY > minY ? maxY : minY + 1;
}

void TouchCalibrator::readDevice() {
    input_event events[64];
    for (;;) {
        const ssize_t n = ::read(fd_, events, sizeof events);
        if (n <= 0) {
            if (n < 0 && errno != EAGAIN && errno != EINTR) {
                finish(NoDevice, QStringLiteral("Touch device went away. Calibration unchanged."));
            }
            return;
        }
        for (size_t i = 0; i < static_cast<size_t>(n) / sizeof(input_event); ++i) {
            feedEvent(events[i].type, events[i].code, events[i].value);
        }
    }
}

void TouchCalibrator::feedEvent(int type, int code, int value) {
    if (finished()) {
        return;
    }
    if (type == EV_ABS && code == ABS_X) {
        x_ = value;
    } else if (type == EV_ABS && code == ABS_Y) {
        y_ = value;
    } else if (type == EV_KEY && code == BTN_TOUCH) {
        if (value) {
            down_ = true;
            xs_.clear();
            ys_.clear();
        } else if (down_) {
            down_ = false;
            release();
        }
    } else if (type == EV_SYN && code == SYN_REPORT && down_) {
        xs_.push_back(x_);
        ys_.push_back(y_);
    }
}

void TouchCalibrator::release() {
    const auto mx = stableMedian(xs_);
    const auto my = stableMedian(ys_);
    if (!mx || !my) {
        return;
    }
    advance((*mx - minX_) / (maxX_ - minX_), (*my - minY_) / (maxY_ - minY_));
}

void TouchCalibrator::advance(double rawX, double rawY) {
    const QPointF t = target();
    points_.push_back({rawX, rawY, t.x() / width_, t.y() / height_});
    ++step_;
    if (step_ < steps()) {
        restartTimer();
        emit changed();
        return;
    }
    const auto solved = solveCalibration(points_, width_, height_);
    if (!solved || solved->maxPixels > kMaxErrorPixels) {
        finish(Inconsistent, QStringLiteral("Taps did not line up. Calibration unchanged."));
        return;
    }
    result_ = *solved;
    finish(Done, QStringLiteral("Calibrated."));
}

void TouchCalibrator::finish(Outcome outcome, const QString& message) {
    if (finished()) {
        return;
    }
    timer_.stop();
    notifier_.reset();
    outcome_ = outcome;
    message_ = message;
    emit changed();
    emit completed(outcome);
}

void TouchCalibrator::restartTimer() {
    secondsLeft_ = timeoutSeconds_;
    timer_.start();
    emit tick();
}

}  // namespace headunit
