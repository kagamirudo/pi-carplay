#include "UiFixture.hpp"

#include <QFile>
#include <QQuickItem>
#include <QTest>

using headunit::test::FakeBluetooth;
using headunit::test::UiFixture;

namespace {

const QStringList kPages = {QStringLiteral("Home"), QStringLiteral("Phone"), QStringLiteral("Cam"),
                            QStringLiteral("Bluetooth"), QStringLiteral("Wi-Fi"), QStringLiteral("Setup")};

// Resistive touch on a 3.5" panel: anything smaller is hard to hit while driving.
constexpr qreal kMinTouchTarget = 32;

}  // namespace

class UiTest : public QObject {
    Q_OBJECT

private:
    std::unique_ptr<UiFixture> ui_;

    void noWarnings() { QVERIFY2(ui_->warnings.isEmpty(), qPrintable(ui_->warnings.join('\n'))); }

private slots:
    void init() {
        ui_ = std::make_unique<UiFixture>();
        QVERIFY(ui_->window());
        QVERIFY(QTest::qWaitForWindowExposed(ui_->window()));
    }

    void cleanup() {
        if (ui_) {
            noWarnings();
        }
        ui_.reset();
    }

    void windowMatchesThePanel() {
        QCOMPARE(ui_->window()->width(), 480);
        QCOMPARE(ui_->window()->height(), 320);
    }

    void everyPageOpens_data() {
        QTest::addColumn<QString>("page");
        for (const QString& page : kPages) {
            QTest::newRow(qPrintable(page)) << page;
        }
    }

    void everyPageOpens() {
        QFETCH(QString, page);
        QVERIFY(ui_->showPage(page));
        QCOMPARE(ui_->currentPage(), page);
        noWarnings();
    }

    void everyButtonIsALargeEnoughTarget() {
        for (const QString& page : kPages) {
            QVERIFY(ui_->showPage(page));
            for (QQuickItem* item : ui_->buttons()) {
                const QString what = page + QStringLiteral(" / ") + item->property("text").toString();
                QVERIFY2(item->width() >= kMinTouchTarget && item->height() >= kMinTouchTarget, qPrintable(what));
                const QRectF rect = item->mapRectToScene(QRectF(0, 0, item->width(), item->height()));
                QVERIFY2(rect.left() >= 0 && rect.right() <= ui_->window()->width() + 0.5, qPrintable(what));
            }
        }
    }

    void everyButtonCanBeTapped() {
        ui_->bluetooth.setDevices({FakeBluetooth::device(QStringLiteral("00:00:00:00:00:01"), QStringLiteral("Car"), true, false)});
        for (const QString& page : kPages) {
            QVERIFY(ui_->showPage(page));
            const auto buttons = ui_->buttons();
            QVERIFY2(!buttons.isEmpty(), qPrintable(page));
            for (QQuickItem* item : buttons) {
                if (kPages.contains(item->property("text").toString())) {
                    continue;
                }
                QVERIFY2(ui_->tap(item), qPrintable(page + QStringLiteral(" / ") + item->property("text").toString()));
            }
            noWarnings();
        }
    }

    void androidAutoButtonResumesProjection() {
        QVERIFY(ui_->showPage(QStringLiteral("Home")));
        QVERIFY(ui_->tap(ui_->button(QStringLiteral("Android Auto"))));
        QTRY_VERIFY_WITH_TIMEOUT(QFile::exists(ui_->resumeMarker()), 5000);
    }

    void settingsSaveFromTheScreen() {
        QVERIFY(ui_->showPage(QStringLiteral("Setup")));
        QQuickItem* segment = nullptr;
        for (QQuickItem* item : ui_->buttons()) {
            if (item->property("text").toString().startsWith(QStringLiteral("Segment:"))) {
                segment = item;
            }
        }
        QVERIFY(segment);
        const QString before = segment->property("text").toString();
        QVERIFY(ui_->tap(segment));
        QVERIFY(segment->property("text").toString() != before);
        QVERIFY(ui_->tap(ui_->button(QStringLiteral("Save"))));
        QCOMPARE(ui_->shell().saveResult(), QStringLiteral("Saved"));
        QVERIFY(std::filesystem::exists(ui_->savePath()));
    }

    void bluetoothListRendersAndDispatches() {
        ui_->bluetooth.setDevices({
            FakeBluetooth::device(QStringLiteral("00:00:00:00:00:01"), QStringLiteral("Phone"), true, true),
            FakeBluetooth::device(QStringLiteral("00:00:00:00:00:02"), QStringLiteral("Car Stereo"), true, false),
            FakeBluetooth::device(QStringLiteral("00:00:00:00:00:03"),
                                  QStringLiteral("A speaker with a very very very long Bluetooth name that must elide"),
                                  false, false),
        });
        QVERIFY(ui_->showPage(QStringLiteral("Bluetooth")));
        const auto count = [this](const QString& text) {
            int n = 0;
            for (QQuickItem* item : ui_->buttons()) {
                n += item->property("text").toString() == text;
            }
            return n;
        };
        QTRY_COMPARE(count(QStringLiteral("Disconnect")), 1);
        QTRY_COMPARE(count(QStringLiteral("Connect")), 2);
        QCOMPARE(count(QStringLiteral("Forget")), 2);

        QVERIFY(ui_->tap(ui_->button(QStringLiteral("Disconnect"))));
        QVERIFY(ui_->tap(ui_->button(QStringLiteral("Connect"))));
        QVERIFY(ui_->tap(ui_->button(QStringLiteral("Scan"))));
        QCOMPARE(ui_->bluetooth.calls,
                 (QStringList{"disconnect 00:00:00:00:00:01", "connect 00:00:00:00:00:02", "scan"}));
        QVERIFY(!ui_->button(QStringLiteral("Scan")));
    }

    void bluetoothListSurvivesOddData() {
        QVERIFY(ui_->showPage(QStringLiteral("Bluetooth")));
        QVariantMap missingFields;
        missingFields.insert(QStringLiteral("address"), QStringLiteral("00:00:00:00:00:09"));
        ui_->bluetooth.setDevices({missingFields, QVariantMap{},
                                   FakeBluetooth::device(QString(), QString(), false, false)});
        QTest::qWait(50);
        ui_->bluetooth.setDevices({});
        QTest::qWait(50);
    }

    void startupNoticeIsShown() {
        ui_->shell().setNotice(QStringLiteral("Settings were reset: test"));
        QVERIFY(ui_->showPage(QStringLiteral("Setup")));
        bool found = false;
        for (QQuickItem* item : ui_->items()) {
            found |= item->property("text").toString() == QStringLiteral("Settings were reset: test") && UiFixture::shown(item);
        }
        QVERIFY(found);
    }
};

QTEST_MAIN(UiTest)
#include "tst_ui.moc"
