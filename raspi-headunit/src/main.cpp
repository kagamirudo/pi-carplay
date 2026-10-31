#include "bluetooth/BluetoothDevices.hpp"
#include "connection/WifiDevices.hpp"
#include "core/ApplicationManager.hpp"
#include "core/ConfigurationManager.hpp"
#include "core/Log.hpp"
#include "core/ShellPresenter.hpp"
#include "input/TouchCalibrator.hpp"
#include "input/TouchInputManager.hpp"

#include <QCommandLineParser>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QScreen>
#include <QTimer>
#include <QUrl>

#include <cstdlib>
#include <filesystem>
#include <iostream>

namespace {

std::filesystem::path defaultSavePath() {
    const char* xdg = std::getenv("XDG_CONFIG_HOME");
    if (xdg && *xdg) {
        return std::filesystem::path(xdg) / "raspi-headunit" / "headunit.json";
    }
    const char* home = std::getenv("HOME");
    if (home && *home) {
        return std::filesystem::path(home) / ".config" / "raspi-headunit" / "headunit.json";
    }
    return "headunit.json";
}

std::filesystem::path shippedConfig() {
    std::error_code ec;
    const auto exe = std::filesystem::read_symlink("/proc/self/exe", ec);
    if (ec) {
        return {};
    }
    const auto candidate = exe.parent_path() / ".." / "config" / "headunit.json";
    if (std::filesystem::is_regular_file(candidate, ec)) {
        return std::filesystem::weakly_canonical(candidate, ec);
    }
    return {};
}

// headunit --calibrate-touch: show targets, print the fitted libinput matrix on
// stdout and exit 0, or exit non-zero (TouchCalibrator::Outcome) with nothing printed.
int calibrateTouch(QGuiApplication& app, const QCommandLineParser& parser) {
    QString device = parser.value(QStringLiteral("touch-device"));
    if (device.isEmpty()) {
        device = QString::fromStdString(headunit::TouchInputManager(headunit::TouchMapping{}).eventNode());
    }
    bool ok = false;
    int timeout = parser.value(QStringLiteral("touch-timeout")).toInt(&ok);
    if (!ok || timeout < 1) {
        timeout = 60;
    }
    const QSize size = app.primaryScreen() ? app.primaryScreen()->geometry().size() : QSize(480, 320);

    headunit::TouchCalibrator calibrator(size.width(), size.height(), timeout);
    QString error;
    if (device.isEmpty() || !calibrator.open(device, &error)) {
        std::cerr << "calibrate: " << (device.isEmpty() ? "no touch device found" : error.toStdString()) << '\n';
        return headunit::TouchCalibrator::NoDevice;
    }

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("calibrator"), &calibrator);
    QObject::connect(&calibrator, &headunit::TouchCalibrator::completed, &app, [&](int outcome) {
        for (const auto& p : calibrator.points()) {
            std::cerr << "calibrate: raw " << p.rawX << ' ' << p.rawY << " target " << p.screenX << ' ' << p.screenY
                      << '\n';
        }
        if (outcome == headunit::TouchCalibrator::Done) {
            std::cerr << "calibrate: error rms " << calibrator.result().rmsPixels << " px, max "
                      << calibrator.result().maxPixels << " px\n";
            std::cout << headunit::formatMatrix(calibrator.result().matrix) << std::endl;
        }
        // Leave the result on screen for a moment, then give the panel back.
        QTimer::singleShot(1500, &app, [outcome] { QCoreApplication::exit(outcome); });
    });
    engine.load(QUrl(QStringLiteral("qrc:/Headunit/ui/Calibrate.qml")));
    if (engine.rootObjects().isEmpty()) {
        return 1;
    }
    return app.exec();
}

}  // namespace

int main(int argc, char* argv[]) {
    qputenv("QT_QUICK_CONTROLS_STYLE", "Basic");

    QGuiApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("headunit"));
    QCoreApplication::setApplicationVersion(QStringLiteral(HEADUNIT_VERSION));

    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addOption({QStringLiteral("config"), QStringLiteral("Config file path"), QStringLiteral("path")});
    parser.addOption({QStringLiteral("smoke"), QStringLiteral("Load the shell and exit")});
    parser.addOption({QStringLiteral("calibrate-touch"),
                      QStringLiteral("Show touch targets and print a libinput calibration matrix")});
    parser.addOption({QStringLiteral("touch-device"), QStringLiteral("evdev node for --calibrate-touch"),
                      QStringLiteral("path")});
    parser.addOption({QStringLiteral("touch-timeout"), QStringLiteral("Seconds to wait for each target (default 60)"),
                      QStringLiteral("seconds")});
    parser.process(app);

    if (parser.isSet(QStringLiteral("calibrate-touch"))) {
        return calibrateTouch(app, parser);
    }

    const bool smoke = parser.isSet(QStringLiteral("smoke"));
    std::filesystem::path savePath = parser.isSet(QStringLiteral("config"))
                                         ? std::filesystem::path(parser.value(QStringLiteral("config")).toStdString())
                                         : defaultSavePath();

    headunit::AppConfig config = headunit::AppConfig::defaults();
    std::string error;
    QString notice;
    headunit::ConfigurationManager loader;
    std::error_code ec;
    if (std::filesystem::exists(savePath, ec)) {
        if (loader.loadFile(savePath, &error)) {
            config = loader.get();
        } else if (parser.isSet(QStringLiteral("config"))) {
            std::cerr << "config: " << error << '\n';
            return 2;
        } else {
            // A driver cannot fix a JSON file: set it aside and start with defaults.
            const auto aside = std::filesystem::path(savePath.string() + ".corrupt");
            std::filesystem::rename(savePath, aside, ec);
            headunit::Logger::error("config", error + (ec ? "" : "; moved to " + aside.string()));
            notice = QStringLiteral("Settings were reset: ") + QString::fromStdString(error);
        }
    } else if (!parser.isSet(QStringLiteral("config"))) {
        const auto shipped = shippedConfig();
        if (!shipped.empty() && loader.loadFile(shipped, &error)) {
            config = loader.get();
        }
    }

    if (const auto level = headunit::parseLogLevel(config.logging.level)) {
        headunit::Logger::setLevel(*level);
    }
    headunit::Logger::setPath(config.logging.path);

    headunit::ApplicationManager manager(std::move(config));
    headunit::ShellPresenter presenter(manager, savePath);
    if (!notice.isEmpty()) {
        presenter.setNotice(notice);
    }

    headunit::BluetoothDevices bluetooth(!qEnvironmentVariableIsSet("HEADUNIT_NO_BLUETOOTH"));
    headunit::WifiDevices wifi(!qEnvironmentVariableIsSet("HEADUNIT_NO_WIFI"));

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("shell"), &presenter);
    engine.rootContext()->setContextProperty(QStringLiteral("bluetooth"), &bluetooth);
    engine.rootContext()->setContextProperty(QStringLiteral("wifi"), &wifi);

    bool failed = false;
    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreated, &app,
        [&](QObject* object, const QUrl&) {
            if (!object) {
                failed = true;
                QCoreApplication::exit(1);
                return;
            }
            if (smoke) {
                QTimer::singleShot(0, &app, &QCoreApplication::quit);
            }
        },
        Qt::QueuedConnection);

    engine.load(QUrl(QStringLiteral("qrc:/Headunit/ui/Main.qml")));
    if (engine.rootObjects().isEmpty() && failed) {
        return 1;
    }
    const int code = app.exec();
    return failed ? 1 : code;
}
