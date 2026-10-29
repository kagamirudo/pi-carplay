#include "connection/PhoneConnectionManager.hpp"

#include <QTest>

using headunit::USBManager;

namespace {

USBManager::Device device(std::string vendor, std::string productId, std::string product, bool hub = false) {
    USBManager::Device item;
    item.vendorId = std::move(vendor);
    item.productId = std::move(productId);
    item.product = std::move(product);
    item.hub = hub;
    return item;
}

}  // namespace

class PhoneTest : public QObject {
    Q_OBJECT

private slots:
    void galaxyInStorageMode() {
        const auto seen = headunit::findAndroidPhone(
            {device("04e8", "6860", "SAMSUNG_Android"), device("046d", "c53f", "USB Receiver")});
        QVERIFY(seen.present);
        QVERIFY(!seen.accessoryMode);
        QCOMPARE(seen.name, std::string("SAMSUNG_Android"));
    }

    void accessoryModeWins() {
        const auto seen =
            headunit::findAndroidPhone({device("04e8", "6860", "SAMSUNG_Android"), device("18d1", "2d00", "Android")});
        QVERIFY(seen.present);
        QVERIFY(seen.accessoryMode);
        QCOMPARE(seen.name, std::string("Android"));
    }

    void everyAccessoryProductId_data() {
        QTest::addColumn<QString>("pid");
        for (const char* pid : {"2d00", "2d01", "2d02", "2d03", "2d04", "2d05"}) {
            QTest::newRow(pid) << pid;
        }
    }

    void everyAccessoryProductId() {
        QFETCH(QString, pid);
        const auto seen = headunit::findAndroidPhone({device("18d1", pid.toStdString(), "Android")});
        QVERIFY(seen.present);
        QVERIFY(seen.accessoryMode);
    }

    void hubAndMouseAreIgnored() {
        const auto seen = headunit::findAndroidPhone(
            {device("1d6b", "0003", "xHCI Host Controller", true), device("046d", "c53f", "USB Receiver")});
        QVERIFY(!seen.present);
    }

    void garbageSysfsValues() {
        const auto seen = headunit::findAndroidPhone({device("", "", ""), device("zzzz", "\n", std::string(4096, 'x'))});
        QVERIFY(!seen.present);
    }

    void linkFollowsThePhone() {
        headunit::PhoneConnectionManager phones;
        phones.lookForPhone();
        phones.updateFromUsb({device("04e8", "6860", "SAMSUNG_Android")});
        QVERIFY(phones.link().transport == headunit::Transport::Usb);
        QVERIFY(phones.link().summary.find("SAMSUNG_Android") != std::string::npos);
        QVERIFY(phones.link().summary.find("not built") == std::string::npos);

        phones.updateFromUsb({});
        QCOMPARE(phones.link().summary, std::string("No Android phone on USB."));
    }

    void plugUnplugChurn() {
        headunit::PhoneConnectionManager phones;
        for (int i = 0; i <= 10000; ++i) {
            if (i % 3 == 0) {
                phones.lookForPhone();
            }
            phones.updateFromUsb(i % 2 ? std::vector<USBManager::Device>{device("18d1", "2d01", "Android")}
                                       : std::vector<USBManager::Device>{});
        }
        QCOMPARE(phones.link().summary, std::string("No Android phone on USB."));
    }
};

QTEST_GUILESS_MAIN(PhoneTest)
#include "tst_phone.moc"
