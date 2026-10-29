#include "ConfigFixtures.hpp"
#include "core/ConfigurationManager.hpp"

#include <QTemporaryDir>
#include <QTest>

#include <unistd.h>

#include <filesystem>
#include <fstream>
#include <string>

using headunit::AppConfig;
using headunit::ConfigurationManager;
using headunit::test::kOverrides;
using headunit::test::kRejects;

namespace {

std::filesystem::path write(const QTemporaryDir& dir, const char* name, const std::string& text) {
    const std::filesystem::path path = std::filesystem::path(dir.path().toStdString()) / name;
    std::ofstream(path, std::ios::binary) << text;
    return path;
}

}  // namespace

class ConfigTest : public QObject {
    Q_OBJECT

private slots:
    void defaults() {
        const auto parsed = ConfigurationManager::parse("{}");
        QVERIFY(parsed.ok);
        QCOMPARE(parsed.config.display.width, 480);
        QCOMPARE(parsed.config.display.height, 320);
        QCOMPARE(parsed.config.display.output, std::string("SPI-1"));
        QVERIFY(parsed.config.display.fullscreen);
        QVERIFY(!parsed.config.dashcam.enabled);
        QCOMPARE(parsed.config.dashcam.width, 1920);
        QCOMPARE(parsed.config.dashcam.segmentSeconds, 120);
        QCOMPARE(parsed.config.dashcam.storageLimitBytes, 0ull);
        QCOMPARE(parsed.config.dashcam.reserveBytes, 512ull * 1024ull * 1024ull);
        QCOMPARE(parsed.config.logging.level, std::string("info"));
    }

    void overridesAndRoundTrip() {
        const auto parsed = ConfigurationManager::parse(kOverrides);
        QVERIFY2(parsed.ok, parsed.error.c_str());
        QVERIFY(!parsed.config.display.fullscreen);
        QCOMPARE(parsed.config.dashcam.segmentSeconds, 60);
        QCOMPARE(parsed.config.dashcam.width, 1920);
        QCOMPARE(parsed.config.dashcam.storageLimitBytes, 137438953472ull);
        QCOMPARE(parsed.config.dashcam.clipsDirectory, std::string("/mnt/footage/clips \"a\""));
        QCOMPARE(parsed.config.logging.level, std::string("warn"));
        QCOMPARE(parsed.config.bluetooth.carDevice, std::string("Car Stereo"));

        const auto again = ConfigurationManager::parse(ConfigurationManager::serialize(parsed.config));
        QVERIFY2(again.ok, again.error.c_str());
        QCOMPARE(again.config.display.fullscreen, parsed.config.display.fullscreen);
        QCOMPARE(again.config.dashcam.clipsDirectory, parsed.config.dashcam.clipsDirectory);
        QCOMPARE(again.config.dashcam.storageLimitBytes, parsed.config.dashcam.storageLimitBytes);
        QCOMPARE(again.config.logging.level, std::string("warn"));
        QCOMPARE(again.config.bluetooth.carDevice, std::string("Car Stereo"));
    }

    void unicodeAndControlCharactersRoundTrip() {
        AppConfig config = AppConfig::defaults();
        config.bluetooth.carDevice = "Xe hơi \t\"\\ \x01 end";
        const auto again = ConfigurationManager::parse(ConfigurationManager::serialize(config));
        QVERIFY2(again.ok, again.error.c_str());
        QCOMPARE(again.config.bluetooth.carDevice, config.bluetooth.carDevice);
    }

    void rejects_data() {
        QTest::addColumn<QString>("text");
        QTest::addColumn<QString>("error");
        for (const auto& [name, text, error] : kRejects) {
            QTest::newRow(name) << QString::fromUtf8(text) << QString::fromUtf8(error);
        }
    }

    void rejects() {
        QFETCH(QString, text);
        QFETCH(QString, error);
        const auto parsed = ConfigurationManager::parse(text.toStdString());
        QVERIFY(!parsed.ok);
        QCOMPARE(QString::fromStdString(parsed.error), error);
    }

    void rejectsTopLevelArray() { QVERIFY(!ConfigurationManager::parse("[]").ok); }

    void deepNestingIsRejectedWithoutRecursingForever() {
        std::string deep;
        for (int i = 0; i < 100000; ++i) {
            deep += "{\"a\":";
        }
        const auto parsed = ConfigurationManager::parse(deep);
        QVERIFY(!parsed.ok);
        QCOMPARE(parsed.error, std::string("JSON nested too deeply"));
    }

    void loadAndSaveFile() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const auto path = std::filesystem::path(dir.path().toStdString()) / "sub" / "headunit.json";

        ConfigurationManager manager;
        AppConfig config = AppConfig::defaults();
        config.dashcam.segmentSeconds = 180;
        manager.reset(config);
        std::string error;
        QVERIFY2(manager.saveFile(path, &error), error.c_str());
        QVERIFY(!std::filesystem::exists(path.string() + ".tmp"));

        ConfigurationManager loaded;
        QVERIFY2(loaded.loadFile(path, &error), error.c_str());
        QCOMPARE(loaded.get().dashcam.segmentSeconds, 180);
    }

    void saveIntoReadOnlyDirectoryFails() {
        if (::geteuid() == 0) {
            QSKIP("root ignores directory permissions");
        }
        QTemporaryDir dir;
        const auto locked = std::filesystem::path(dir.path().toStdString()) / "locked";
        std::filesystem::create_directory(locked);
        std::filesystem::permissions(locked, std::filesystem::perms::owner_read | std::filesystem::perms::owner_exec);
        ConfigurationManager manager;
        std::string error;
        QVERIFY(!manager.saveFile(locked / "headunit.json", &error));
        QVERIFY(!error.empty());
        std::filesystem::permissions(locked, std::filesystem::perms::owner_all);
    }

    void saveUnderAFileFails() {
        QTemporaryDir dir;
        const auto blocker = write(dir, "blocker", "x");
        ConfigurationManager manager;
        std::string error;
        QVERIFY(!manager.saveFile(blocker / "headunit.json", &error));
        QVERIFY(!error.empty());
        QVERIFY(!manager.saveFile(blocker / "sub" / "headunit.json", &error));
        QVERIFY(!error.empty());
    }

    void missingFileFails() {
        ConfigurationManager manager;
        std::string error;
        QVERIFY(!manager.loadFile("/nonexistent/headunit.json", &error));
        QVERIFY(error.find("could not read") != std::string::npos);
        QCOMPARE(manager.get().display.width, 480);
    }

    void oversizedFileIsRejected() {
        QTemporaryDir dir;
        const auto path = write(dir, "big.json", "{\"pad\":\"" + std::string(ConfigurationManager::kMaxFileBytes, 'x') + "\"}");
        ConfigurationManager manager;
        std::string error;
        QVERIFY(!manager.loadFile(path, &error));
        QVERIFY(error.find("larger than") != std::string::npos);
    }

    void failedLoadKeepsPreviousConfig() {
        QTemporaryDir dir;
        ConfigurationManager manager;
        AppConfig config = AppConfig::defaults();
        config.display.width = 800;
        manager.reset(config);
        std::string error;
        QVERIFY(!manager.loadFile(write(dir, "bad.json", "{\"display\":"), &error));
        QCOMPARE(manager.get().display.width, 800);
    }

    void shippedConfigParses() {
        ConfigurationManager manager;
        std::string error;
        QVERIFY2(manager.loadFile(HEADUNIT_SOURCE_DIR "/config/headunit.json", &error), error.c_str());
    }
};

QTEST_GUILESS_MAIN(ConfigTest)
#include "tst_config.moc"
