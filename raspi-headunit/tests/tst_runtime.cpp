// Logging, projection state and audio routing: the pieces every other part
// of the shell leans on at run time.
#include "audio/AudioRouter.hpp"
#include "core/Log.hpp"
#include "projection/ProjectionManager.hpp"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QTest>

#include <thread>
#include <vector>

using namespace headunit;

namespace {

QList<QJsonObject> readLog(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }
    QList<QJsonObject> lines;
    for (const QByteArray& line : file.readAll().split('\n')) {
        if (line.isEmpty()) {
            continue;
        }
        QJsonParseError error;
        const QJsonDocument doc = QJsonDocument::fromJson(line, &error);
        if (error.error != QJsonParseError::NoError || !doc.isObject()) {
            qWarning("not a JSON log line: %s", line.constData());
            return {};
        }
        lines << doc.object();
    }
    return lines;
}

}  // namespace

class RuntimeTest : public QObject {
    Q_OBJECT

private:
    QTemporaryDir dir_;

private slots:
    void cleanup() {
        Logger::setPath({});
        Logger::setLevel(LogLevel::Info);
    }

    void levelNames() {
        for (LogLevel level : {LogLevel::Debug, LogLevel::Info, LogLevel::Warn, LogLevel::Error}) {
            QCOMPARE(parseLogLevel(logLevelName(level)), std::optional<LogLevel>(level));
        }
        QCOMPARE(parseLogLevel("warning"), std::optional<LogLevel>(LogLevel::Warn));
        QVERIFY(!parseLogLevel("").has_value());
        QVERIFY(!parseLogLevel("INFO ").has_value());
    }

    void writesOneJsonObjectPerLine() {
        const QString path = dir_.filePath(QStringLiteral("json.log"));
        Logger::setPath(path.toStdString());
        Logger::setLevel(LogLevel::Debug);
        Logger::event(LogLevel::Debug, "test", "quote \" backslash \\ newline \n tab \t bell \x07 end",
                      {{"key \"1\"", "value\n"}, {"empty", ""}});
        Logger::info("test", "second");
        const auto lines = readLog(path);
        QCOMPARE(lines.size(), 2);
        QCOMPARE(lines[0].value(QStringLiteral("msg")).toString(),
                 QStringLiteral("quote \" backslash \\ newline \n tab \t bell \x07 end"));
        QCOMPARE(lines[0].value(QStringLiteral("fields")).toObject().value(QStringLiteral("key \"1\"")).toString(),
                 QStringLiteral("value\n"));
        QCOMPARE(lines[1].value(QStringLiteral("level")).toString(), QStringLiteral("info"));
        QVERIFY(lines[1].value(QStringLiteral("ts")).toString().endsWith('Z'));
    }

    void filtersBelowLevel() {
        const QString path = dir_.filePath(QStringLiteral("level.log"));
        Logger::setPath(path.toStdString());
        Logger::setLevel(LogLevel::Warn);
        Logger::info("test", "hidden");
        Logger::warn("test", "shown");
        Logger::error("test", "shown");
        QCOMPARE(readLog(path).size(), 2);
    }

    void unwritableLogPathIsSurvivable() {
        Logger::setPath("/proc/no-such-dir/headunit.log");
        Logger::error("test", "still goes to stderr");
        Logger::setPath({});
    }

    void concurrentWritersDoNotInterleave() {
        const QString path = dir_.filePath(QStringLiteral("threads.log"));
        Logger::setPath(path.toStdString());
        std::vector<std::thread> threads;
        for (int t = 0; t < 8; ++t) {
            threads.emplace_back([t] {
                for (int i = 0; i < 250; ++i) {
                    Logger::info("thread", std::string(static_cast<std::size_t>(50 + t * 10), static_cast<char>('a' + t)));
                }
            });
        }
        for (auto& thread : threads) {
            thread.join();
        }
        QCOMPARE(readLog(path).size(), 2000);
    }

    void projectionStartsInactive() {
        ProjectionManager projection;
        QVERIFY(projection.active() == nullptr);
        QCOMPARE(std::string(projection.screenOwner()), std::string("shell"));
        for (ProjectionReceiver* receiver : {&projection.androidAuto(), &projection.carPlay()}) {
            QVERIFY(!receiver->id().empty());
            QVERIFY(!receiver->displayName().empty());
            QVERIFY(!receiver->detail().empty());
            QVERIFY(receiver->session() == SessionState::Inactive);
            receiver->stop();
            QVERIFY(receiver->session() != SessionState::Active);
        }
        const ProjectionManager& constProjection = projection;
        QCOMPARE(constProjection.carPlay().displayName(), projection.carPlay().displayName());
    }

    void carPlayNeverClaimsToWorkWithoutAnAdapter() {
        ProjectionManager projection;
        QVERIFY(!projection.carPlay().start());
        QVERIFY(projection.carPlay().session() != SessionState::Active);
        QVERIFY(projection.carPlay().maturity() != FeatureMaturity::Working);
        QVERIFY(projection.active() == nullptr);
    }

    void stateNames() {
        QCOMPARE(std::string(maturityName(FeatureMaturity::Planned)), std::string("planned"));
        QCOMPARE(std::string(maturityName(FeatureMaturity::Experimental)), std::string("experimental"));
        QCOMPARE(std::string(maturityName(FeatureMaturity::Working)), std::string("working"));
        QCOMPARE(std::string(sessionName(SessionState::Inactive)), std::string("inactive"));
        QCOMPARE(std::string(sessionName(SessionState::Starting)), std::string("starting"));
        QCOMPARE(std::string(sessionName(SessionState::Active)), std::string("active"));
        QCOMPARE(std::string(sessionName(SessionState::Failed)), std::string("failed"));
    }

    void audioRoutes() {
        AudioRouter audio;
        QVERIFY(audio.route() == AudioRoute::None);
        for (AudioRoute route : {AudioRoute::LocalUi, AudioRoute::Projection, AudioRoute::None}) {
            audio.setRoute(route);
            QVERIFY(audio.route() == route);
            QVERIFY(std::string(audioRouteName(route)).size() > 0);
            QVERIFY(!audio.detail().empty());
        }
    }
};

QTEST_GUILESS_MAIN(RuntimeTest)
#include "tst_runtime.moc"
