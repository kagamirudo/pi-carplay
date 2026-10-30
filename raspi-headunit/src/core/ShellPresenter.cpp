#include "core/ShellPresenter.hpp"

#include "core/Log.hpp"

#include <QProcess>
#include <QStringList>
#include <QTimer>

#include <array>
#include <cstdlib>
#include <fstream>
#include <limits>
#include <string>
#include <system_error>
#include <vector>

namespace headunit {
namespace {

QString receiverLine(const ProjectionReceiver& receiver) {
    return QString::fromStdString(std::string(receiver.displayName()) + ": " + maturityName(receiver.maturity()) +
                                  " (" + sessionName(receiver.session()) + ")");
}

std::string unescapeMount(std::string text) {
    std::string out;
    out.reserve(text.size());
    for (std::size_t i = 0; i < text.size(); ++i) {
        if (text[i] == '\\' && i + 3 < text.size() && text[i + 1] == '0' && text[i + 2] == '4' && text[i + 3] == '0') {
            out.push_back(' ');
            i += 3;
        } else {
            out.push_back(text[i]);
        }
    }
    return out;
}

std::vector<VolumeMount> readMounts(std::string* osSource) {
    std::vector<VolumeMount> mounts;
    std::ifstream in("/proc/mounts");
    std::string source;
    std::string point;
    while (in >> source >> point) {
        in.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        VolumeMount mount;
        mount.source = unescapeMount(source);
        mount.mountPoint = unescapeMount(point);
        if (mount.mountPoint == "/" && osSource) {
            *osSource = mount.source;
        }
        mounts.push_back(std::move(mount));
    }
    return mounts;
}

}  // namespace

ShellPresenter::ShellPresenter(ApplicationManager& app, std::filesystem::path savePath, QObject* parent)
    : QObject(parent), app_(app), savePath_(std::move(savePath)) {
    auto* timer = new QTimer(this);
    timer->setInterval(3000);
    connect(timer, &QTimer::timeout, this, &ShellPresenter::refresh);
    timer->start();
    refresh();
}

QString ShellPresenter::version() const { return QStringLiteral(HEADUNIT_VERSION); }

QString ShellPresenter::formatBytes(std::uint64_t bytes) const {
    constexpr std::uint64_t gib = 1024ull * 1024ull * 1024ull;
    constexpr std::uint64_t mib = 1024ull * 1024ull;
    if (bytes == 0) {
        return QStringLiteral("not set");
    }
    if (bytes % gib == 0) {
        return QString::number(bytes / gib) + QStringLiteral(" GiB");
    }
    if (bytes % mib == 0) {
        return QString::number(bytes / mib) + QStringLiteral(" MiB");
    }
    return QString::number(static_cast<qulonglong>(bytes)) + QStringLiteral(" B");
}

void ShellPresenter::refresh() {
    const auto usb = app_.usb().listDevices();
    app_.phones().updateFromUsb(usb);
    const PhoneLink& link = app_.phones().link();
    linkState_ = QString::fromUtf8(linkStateName(link.state));
    linkDetail_ = QString::fromStdString(link.summary);

    int shown = 0;
    QStringList names;
    for (const auto& device : usb) {
        if (device.hub) {
            continue;
        }
        if (shown == 3) {
            break;
        }
        names.push_back(QString::fromStdString(device.product));
        ++shown;
    }
    if (names.isEmpty()) {
        usbSummary_ = QStringLiteral("No USB device with a product name.");
    } else {
        usbSummary_ = names.join(QStringLiteral(", "));
    }

    const auto wifi = app_.wifi().status();
    wifiSummary_ = wifi.exists ? QString::fromStdString(wifi.interfaceName + " " + wifi.operstate)
                               : QStringLiteral("wlan0 absent");

    const StereoLink stereo = app_.bluetooth().status();
    bluetoothSummary_ = QString::fromStdString(stereo.detail);

    audioSummary_ = QString::fromUtf8(audioRouteName(app_.audio().route())) + QStringLiteral(" / ") +
                    QString::fromUtf8(app_.audio().sink().data(), static_cast<int>(app_.audio().sink().size()));
    audioSummary_ += QStringLiteral(". ") + QString::fromUtf8(app_.audio().detail().data(),
                                                              static_cast<int>(app_.audio().detail().size()));

    const AndroidSighting& android = app_.phones().android();
    if (android.present) {
        androidAuto_ = QStringLiteral("Android Auto: ") + QString::fromStdString(android.name) +
                       (android.accessoryMode ? QStringLiteral(" is in accessory mode")
                                              : QStringLiteral(" is plugged in"));
    } else {
        androidAuto_ = QStringLiteral("Android Auto: no phone plugged in");
    }
    carPlay_ = receiverLine(app_.projection().carPlay());
    screenOwner_ = QStringLiteral("Screen: ") + QString::fromUtf8(app_.projection().screenOwner().data(),
                                                                  static_cast<int>(app_.projection().screenOwner().size()));
    videoSummary_ = QString::fromUtf8(app_.video().detail().data(), static_cast<int>(app_.video().detail().size()));

    const DashcamStatus& cam = app_.dashcam().status();
    recordState_ = QString::fromUtf8(recordStateName(cam.state));
    recordDetail_ = QString::fromStdString(cam.detail);
    recordGeometry_ = QString::number(cam.width) + QStringLiteral("×") + QString::number(cam.height) +
                      QStringLiteral(" @ ") + QString::number(cam.fps);
    focusSummary_ = QStringLiteral("Focus bias: ") + QString::fromUtf8(focusBiasName(cam.focus)) +
                    QStringLiteral(" (not applied to the lens)");
    lastClip_ = cam.lastClip.empty() ? QStringLiteral("none") : QString::fromStdString(cam.lastClip);
    segmentLabel_ = QString::number(cam.segmentSeconds) + QStringLiteral(" s segments");

    const DashcamConfig& dash = app_.config().dashcam;
    if (dash.clipsDirectory.empty() || dash.storageLimitBytes == 0) {
        storageSummary_ = QStringLiteral("No footage disk. The OS card is not used. Reserve ") +
                          formatBytes(dash.reserveBytes) + QStringLiteral(".");
    } else {
        std::string osSource;
        const std::vector<VolumeMount> mounts = readMounts(&osSource);
        std::string reason;
        const bool allowed = app_.storage().footageVolumeAllowed(dash.clipsDirectory, osSource, mounts, &reason);
        storageSummary_ = allowed ? QString::fromStdString(dash.clipsDirectory) + QStringLiteral(" is a separate volume")
                                  : QString::fromStdString(reason);
    }
    storageLimitLabel_ = formatBytes(dash.storageLimitBytes);
    logLevel_ = QString::fromStdString(app_.config().logging.level);

    const DisplayConfig& display = app_.config().display;
    displaySummary_ = QString::number(display.width) + QStringLiteral("×") + QString::number(display.height) +
                      QStringLiteral(" ") + QString::fromStdString(display.output) +
                      (display.fullscreen ? QStringLiteral(" fullscreen") : QStringLiteral(" windowed"));

    const TouchMapping& touch = app_.touch().mapping();
    const std::string node = app_.touch().eventNode();
    touchSummary_ = QString::fromStdString(touch.deviceName) + QStringLiteral(" → ") +
                    QString::fromStdString(touch.output) +
                    QStringLiteral(", mouse emulation ") + (touch.mouseEmulation ? QStringLiteral("on") : QStringLiteral("off"));
    if (!node.empty()) {
        touchSummary_ += QStringLiteral(" (") + QString::fromStdString(node) + QStringLiteral(")");
    }
    if (!touch.forwardingToPhone) {
        touchSummary_ += QStringLiteral(". Not forwarded to a phone.");
    }

    homePhone_ = linkDetail_;
    homeCamera_ = recordState_.toUpper() + QStringLiteral(" · ") + recordDetail_;
    homeDisplay_ = displaySummary_;
    previewOpen_ = readPreviewFlag();
    emit changed();
}

std::filesystem::path ShellPresenter::previewFlagPath() const {
    if (const char* home = std::getenv("HOME")) {
        return std::filesystem::path(home) / ".local/state/pi-carplay/dashcam-preview";
    }
    return "/tmp/pi-carplay-dashcam-preview";
}

bool ShellPresenter::readPreviewFlag() const {
    std::ifstream in(previewFlagPath());
    std::string word;
    in >> word;
    return word == "on";
}

void ShellPresenter::togglePreview() {
    const bool next = !previewOpen_;
    const auto path = previewFlagPath();
    std::error_code error;
    std::filesystem::create_directories(path.parent_path(), error);
    std::ofstream out(path, std::ios::trunc);
    if (!out) {
        Logger::warn("dashcam", "could not write the preview flag");
        return;
    }
    out << (next ? "on\n" : "off\n");
    previewOpen_ = next;
    emit changed();
}

void ShellPresenter::lookForPhone() {
    app_.phones().lookForPhone();
    Logger::info("connection", "look for phone");
    refresh();
}

void ShellPresenter::cycleSegment() {
    const int current = app_.config().dashcam.segmentSeconds;
    int next = 60;
    if (current < 60) {
        next = 60;
    } else if (current < 120) {
        next = 120;
    } else if (current < 180) {
        next = 180;
    }
    app_.setSegmentSeconds(next);
    saveResult_ = QStringLiteral("Unsaved change");
    refresh();
}

void ShellPresenter::cycleLogLevel() {
    const std::string current = app_.config().logging.level;
    const char* next = "info";
    if (current == "debug") {
        next = "info";
    } else if (current == "info") {
        next = "warn";
    } else if (current == "warn") {
        next = "error";
    } else if (current == "error") {
        next = "debug";
    }
    app_.setLogLevel(next);
    saveResult_ = QStringLiteral("Unsaved change");
    refresh();
}

void ShellPresenter::adjustStorageLimit(int deltaSteps) {
    constexpr std::array<std::uint64_t, 6> steps = {
        0,
        32ull << 30,
        64ull << 30,
        128ull << 30,
        256ull << 30,
        512ull << 30,
    };
    const std::uint64_t current = app_.config().dashcam.storageLimitBytes;
    std::uint64_t chosen = current;
    if (deltaSteps > 0) {
        for (std::uint64_t step : steps) {
            if (step > current) {
                chosen = step;
                break;
            }
        }
    } else if (deltaSteps < 0) {
        for (auto it = steps.rbegin(); it != steps.rend(); ++it) {
            if (*it < current) {
                chosen = *it;
                break;
            }
        }
    }
    if (chosen != current) {
        app_.setStorageLimitBytes(chosen);
        saveResult_ = QStringLiteral("Unsaved change");
    }
    refresh();
}

void ShellPresenter::saveSettings() {
    std::string error;
    if (!app_.saveConfig(savePath_, &error)) {
        saveResult_ = QString::fromStdString(error);
        Logger::error("config", error);
    } else {
        saveResult_ = QStringLiteral("Saved");
        Logger::info("config", "saved");
    }
    emit changed();
}

void ShellPresenter::setCarMode(bool on) {
    carMode_ = on;
    if (on) {
        openAndroidAuto();
    }
    emit changed();
}

void ShellPresenter::openAndroidAuto() {
    if (resumeCommand_.isEmpty()) {
        return;
    }
    if (!QProcess::startDetached(resumeCommand_.first(), resumeCommand_.mid(1))) {
        Logger::warn("projection", "could not signal autoapp");
    }
}

void ShellPresenter::reconnectPhone() {
    setNotice(QStringLiteral("Reconnecting the phone…"));
    if (!QProcess::startDetached(QStringLiteral("aa-restart.sh"), QStringList{})) {
        Logger::warn("projection", "could not run aa-restart.sh");
    }
}

void ShellPresenter::setResumeCommand(QStringList command) { resumeCommand_ = std::move(command); }

void ShellPresenter::setNotice(const QString& text) {
    saveResult_ = text;
    emit changed();
}

void ShellPresenter::requestRecord() {
    const bool started = app_.dashcam().start();
    if (!started) {
        Logger::warn("dashcam", app_.dashcam().status().detail);
    }
    refresh();
}

}  // namespace headunit
