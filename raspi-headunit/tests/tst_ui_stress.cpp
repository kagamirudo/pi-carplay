// Monkey test: thousands of seeded random taps across every page while the
// backends change state underneath. Fails on any QML warning, crash, or
// unbounded memory growth.
#include "UiFixture.hpp"

#include <QTest>

#include <unistd.h>

#include <fstream>
#include <random>

using headunit::test::FakeBluetooth;
using headunit::test::UiFixture;

namespace {

long residentKiB() {
    std::ifstream in("/proc/self/statm");
    long pages = 0;
    long resident = 0;
    in >> pages >> resident;
    return resident * (sysconf(_SC_PAGESIZE) / 1024);
}

QVariantList randomDevices(std::mt19937& rng) {
    QVariantList list;
    const int count = static_cast<int>(rng() % 12);
    for (int i = 0; i < count; ++i) {
        const QString address = QStringLiteral("00:00:00:00:00:%1").arg(i, 2, 16, QLatin1Char('0')).toUpper();
        list << FakeBluetooth::device(address, QStringLiteral("Device %1").arg(rng() % 1000), rng() % 2, rng() % 4 == 0);
    }
    return list;
}

}  // namespace

class UiStressTest : public QObject {
    Q_OBJECT

private slots:
    void randomTaps() {
        const int taps = qEnvironmentVariableIntValue("HEADUNIT_STRESS_TAPS") > 0
                             ? qEnvironmentVariableIntValue("HEADUNIT_STRESS_TAPS")
                             : 3000;
        UiFixture ui;
        QVERIFY(ui.window());
        QVERIFY(QTest::qWaitForWindowExposed(ui.window()));
        std::mt19937 rng(0xca5);

        long baseline = 0;
        for (int i = 0; i < taps; ++i) {
            if (i % 25 == 0) {
                ui.bluetooth.setDevices(randomDevices(rng));
            }
            if (i % 40 == 0) {
                ui.shell().refresh();
            }
            const auto buttons = ui.buttons();
            QVERIFY(!buttons.isEmpty());
            QQuickItem* item = buttons.at(static_cast<int>(rng() % buttons.size()));
            ui.tap(item);
            if (i % 100 == 0) {
                QTest::qWait(1);
            }
            if (i == taps / 5) {
                baseline = residentKiB();
            }
            QVERIFY2(ui.warnings.isEmpty(), qPrintable(QStringLiteral("after %1 taps:\n").arg(i) + ui.warnings.join('\n')));
        }

        const long grown = residentKiB() - baseline;
        qInfo("resident memory grew %ld KiB over %d taps", grown, taps - taps / 5);
#ifdef HEADUNIT_SANITIZED
        QSKIP("ASan quarantines freed memory; its leak checker covers this build");
#endif
        const long limitKiB = qEnvironmentVariableIntValue("HEADUNIT_STRESS_MAX_GROWTH_KIB") > 0
                                  ? qEnvironmentVariableIntValue("HEADUNIT_STRESS_MAX_GROWTH_KIB")
                                  : 8 * 1024;
        QVERIFY2(grown < limitKiB, qPrintable(QStringLiteral("grew %1 KiB").arg(grown)));
    }

    void rapidPageSwitching() {
        UiFixture ui;
        QVERIFY(QTest::qWaitForWindowExposed(ui.window()));
        const QStringList pages = {QStringLiteral("Home"), QStringLiteral("Phone"), QStringLiteral("Cam"),
                                   QStringLiteral("BT"), QStringLiteral("Setup")};
        for (int i = 0; i < 1000; ++i) {
            QVERIFY(ui.showPage(pages.at(i % pages.size())));
        }
        QVERIFY2(ui.warnings.isEmpty(), qPrintable(ui.warnings.join('\n')));
    }
};

QTEST_MAIN(UiStressTest)
#include "tst_ui_stress.moc"
