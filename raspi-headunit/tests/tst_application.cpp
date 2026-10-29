#include "core/ApplicationManager.hpp"

#include <QTemporaryDir>
#include <QTest>

using headunit::AppConfig;
using headunit::ApplicationManager;

class ApplicationTest : public QObject {
    Q_OBJECT

private slots:
    void segmentAcceptsOnlySupportedLengths() {
        ApplicationManager app(AppConfig::defaults());
        for (int bad : {-1, 0, 59, 61, 100000}) {
            QVERIFY(!app.setSegmentSeconds(bad));
            QCOMPARE(app.config().dashcam.segmentSeconds, 120);
        }
        for (int good : {60, 120, 180}) {
            QVERIFY(app.setSegmentSeconds(good));
            QCOMPARE(app.config().dashcam.segmentSeconds, good);
            QCOMPARE(app.dashcam().status().segmentSeconds, good);
        }
    }

    void logLevelIsNormalised() {
        ApplicationManager app(AppConfig::defaults());
        QVERIFY(!app.setLogLevel("loud"));
        QCOMPARE(app.config().logging.level, std::string("info"));
        QVERIFY(app.setLogLevel("warning"));
        QCOMPARE(app.config().logging.level, std::string("warn"));
        app.setLogLevel("info");
    }

    void storageLimitResetsRecorder() {
        ApplicationManager app(AppConfig::defaults());
        QVERIFY(app.setStorageLimitBytes(64ull << 30));
        QCOMPARE(app.config().dashcam.storageLimitBytes, 64ull << 30);
        QVERIFY(app.dashcam().status().state == headunit::RecordState::Idle);
    }

    void recorderRefusesWithoutFootageDisk() {
        ApplicationManager app(AppConfig::defaults());
        QVERIFY(!app.dashcam().start());
        QVERIFY(!app.dashcam().status().encoderStarted);
        QVERIFY(app.dashcam().status().detail.find("OS card") != std::string::npos);
    }

    void saveRoundTrip() {
        QTemporaryDir dir;
        ApplicationManager app(AppConfig::defaults());
        app.setSegmentSeconds(60);
        const auto path = std::filesystem::path(dir.path().toStdString()) / "headunit.json";
        std::string error;
        QVERIFY2(app.saveConfig(path, &error), error.c_str());
        headunit::ConfigurationManager loaded;
        QVERIFY(loaded.loadFile(path, &error));
        QCOMPARE(loaded.get().dashcam.segmentSeconds, 60);
    }

    void hardwareProbesDoNotThrow() {
        ApplicationManager app(AppConfig::defaults());
        app.usb().listDevices();
        app.wifi().status();
        app.bluetooth().status();
        app.touch().eventNode();
    }
};

QTEST_GUILESS_MAIN(ApplicationTest)
#include "tst_application.moc"
