#include "bluetooth/BluetoothDevices.hpp"
#include "core/ApplicationManager.hpp"
#include "core/ConfigurationManager.hpp"
#include "core/Log.hpp"
#include "core/ShellPresenter.hpp"

#include <QCommandLineParser>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
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
    parser.process(app);

    const bool smoke = parser.isSet(QStringLiteral("smoke"));
    std::filesystem::path savePath = parser.isSet(QStringLiteral("config"))
                                         ? std::filesystem::path(parser.value(QStringLiteral("config")).toStdString())
                                         : defaultSavePath();

    headunit::AppConfig config = headunit::AppConfig::defaults();
    std::string error;
    headunit::ConfigurationManager loader;
    if (std::filesystem::is_regular_file(savePath)) {
        if (!loader.loadFile(savePath, &error)) {
            std::cerr << "config: " << error << '\n';
            return 1;
        }
        config = loader.get();
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

    headunit::BluetoothDevices bluetooth;

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("shell"), &presenter);
    engine.rootContext()->setContextProperty(QStringLiteral("bluetooth"), &bluetooth);

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
