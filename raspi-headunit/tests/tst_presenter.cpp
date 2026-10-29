#include "core/ShellPresenter.hpp"

#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

#include <filesystem>

using headunit::AppConfig;
using headunit::ApplicationManager;
using headunit::ShellPresenter;

class PresenterTest : public QObject {
    Q_OBJECT

private:
    QTemporaryDir dir_;

    std::filesystem::path savePath() const { return std::filesystem::path(dir_.path().toStdString()) / "headunit.json"; }

private slots:
    void initialStateIsFilled() {
        ApplicationManager app(AppConfig::defaults());
        ShellPresenter shell(app, savePath());
        QVERIFY(!shell.linkState().isEmpty());
        QVERIFY(!shell.homePhone().isEmpty());
        QVERIFY(!shell.homeDisplay().isEmpty());
        QCOMPARE(shell.displaySummary(), QStringLiteral("480×320 SPI-1 fullscreen"));
        QCOMPARE(shell.version(), QStringLiteral(HEADUNIT_VERSION));
        QCOMPARE(shell.saveResult(), QStringLiteral("Not saved yet"));
    }

    void segmentCycles() {
        ApplicationManager app(AppConfig::defaults());
        ShellPresenter shell(app, savePath());
        QStringList seen;
        for (int i = 0; i < 4; ++i) {
            shell.cycleSegment();
            seen << shell.segmentLabel();
        }
        QCOMPARE(seen, (QStringList{"180 s segments", "60 s segments", "120 s segments", "180 s segments"}));
        QCOMPARE(shell.saveResult(), QStringLiteral("Unsaved change"));
    }

    void logLevelCycles() {
        ApplicationManager app(AppConfig::defaults());
        ShellPresenter shell(app, savePath());
        QStringList seen;
        for (int i = 0; i < 4; ++i) {
            shell.cycleLogLevel();
            seen << shell.logLevel();
        }
        QCOMPARE(seen, (QStringList{"warn", "error", "debug", "info"}));
    }

    void storageLimitIsClamped() {
        ApplicationManager app(AppConfig::defaults());
        ShellPresenter shell(app, savePath());
        shell.adjustStorageLimit(-1);
        QCOMPARE(shell.storageLimitLabel(), QStringLiteral("not set"));
        QStringList seen;
        for (int i = 0; i < 7; ++i) {
            shell.adjustStorageLimit(1);
            seen << shell.storageLimitLabel();
        }
        QCOMPARE(seen, (QStringList{"32 GiB", "64 GiB", "128 GiB", "256 GiB", "512 GiB", "512 GiB", "512 GiB"}));
        shell.adjustStorageLimit(0);
        QCOMPARE(shell.storageLimitLabel(), QStringLiteral("512 GiB"));
        for (int i = 0; i < 10; ++i) {
            shell.adjustStorageLimit(-1);
        }
        QCOMPARE(shell.storageLimitLabel(), QStringLiteral("not set"));
    }

    void saveWritesTheFile() {
        ApplicationManager app(AppConfig::defaults());
        ShellPresenter shell(app, savePath());
        shell.cycleSegment();
        QSignalSpy spy(&shell, &ShellPresenter::changed);
        shell.saveSettings();
        QCOMPARE(spy.count(), 1);
        QCOMPARE(shell.saveResult(), QStringLiteral("Saved"));
        headunit::ConfigurationManager loaded;
        std::string error;
        QVERIFY2(loaded.loadFile(savePath(), &error), error.c_str());
        QCOMPARE(loaded.get().dashcam.segmentSeconds, 180);
    }

    void saveFailureIsReportedNotThrown() {
        ApplicationManager app(AppConfig::defaults());
        ShellPresenter shell(app, "/proc/headunit-cannot-write/headunit.json");
        shell.saveSettings();
        QVERIFY(shell.saveResult().startsWith(QStringLiteral("could not")));
    }

    void recordWithoutDiskExplainsWhy() {
        ApplicationManager app(AppConfig::defaults());
        ShellPresenter shell(app, savePath());
        shell.requestRecord();
        QCOMPARE(shell.recordState(), QStringLiteral("idle"));
        QVERIFY(shell.recordDetail().contains(QStringLiteral("OS card")));
        QVERIFY(shell.storageSummary().startsWith(QStringLiteral("No footage disk")));
    }

    void openAndroidAutoRunsResumeCommand() {
        ApplicationManager app(AppConfig::defaults());
        ShellPresenter shell(app, savePath());
        const QString marker = dir_.filePath(QStringLiteral("resumed"));
        shell.setResumeCommand({QStringLiteral("/bin/sh"), QStringLiteral("-c"), QStringLiteral("touch \"$0\""), marker});
        shell.openAndroidAuto();
        QTRY_VERIFY_WITH_TIMEOUT(QFile::exists(marker), 5000);
    }

    void openAndroidAutoWithMissingProgramIsHarmless() {
        ApplicationManager app(AppConfig::defaults());
        ShellPresenter shell(app, savePath());
        shell.setResumeCommand({QStringLiteral("/nonexistent/pkill")});
        shell.openAndroidAuto();
        shell.setResumeCommand({});
        shell.openAndroidAuto();
    }

    void noticeReplacesSaveResult() {
        ApplicationManager app(AppConfig::defaults());
        ShellPresenter shell(app, savePath());
        QSignalSpy spy(&shell, &ShellPresenter::changed);
        shell.setNotice(QStringLiteral("Settings were reset"));
        QCOMPARE(spy.count(), 1);
        QCOMPARE(shell.saveResult(), QStringLiteral("Settings were reset"));
    }

    void periodicRefreshKeepsRunning() {
        ApplicationManager app(AppConfig::defaults());
        ShellPresenter shell(app, savePath());
        QSignalSpy spy(&shell, &ShellPresenter::changed);
        QVERIFY(spy.wait(4000));
    }

    void manyRefreshes() {
        ApplicationManager app(AppConfig::defaults());
        ShellPresenter shell(app, savePath());
        for (int i = 0; i < 500; ++i) {
            shell.refresh();
            shell.lookForPhone();
        }
        QVERIFY(!shell.linkState().isEmpty());
    }
};

QTEST_GUILESS_MAIN(PresenterTest)
#include "tst_presenter.moc"
