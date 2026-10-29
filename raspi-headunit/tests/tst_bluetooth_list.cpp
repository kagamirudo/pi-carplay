#include "bluetooth/BluetoothDevices.hpp"
#include "bluetooth/DeviceList.hpp"

#include <QSignalSpy>
#include <QTest>
#include <QVariantMap>

using namespace headunit;

class BluetoothListTest : public QObject {
    Q_OBJECT

private slots:
    void parsesBluetoothctlOutput() {
        const QString output = QStringLiteral(
            "[CHG] Controller 2C:CF:67:00:00:01 Discovering: yes\n"
            "Device 00:1A:7D:DA:71:13 Car Stereo\n"
            "  Device 11:22:33:44:55:66   JBL Flip 5  \n"
            "Device aa:bb:cc:dd:ee:ff lower case\n"
            "Device 00:1A:7D:DA:71 short\n"
            "Device 77:88:99:AA:BB:CC\n"
            "garbage\n\n");
        const auto entries = parseDeviceList(output);
        QCOMPARE(entries.size(), 3);
        QCOMPARE(entries[0].address, QStringLiteral("00:1A:7D:DA:71:13"));
        QCOMPARE(entries[0].name, QStringLiteral("Car Stereo"));
        QCOMPARE(entries[1].name, QStringLiteral("JBL Flip 5"));
        QCOMPARE(entries[2].name, QString());
    }

    void unnamedDetection() {
        QVERIFY(isUnnamed({QStringLiteral("11:22:33:44:55:66"), QString()}));
        QVERIFY(isUnnamed({QStringLiteral("11:22:33:44:55:66"), QStringLiteral("11-22-33-44-55-66")}));
        QVERIFY(!isUnnamed({QStringLiteral("11:22:33:44:55:66"), QStringLiteral("Car")}));
    }

    void addressValidation_data() {
        QTest::addColumn<QString>("address");
        QTest::addColumn<bool>("valid");
        QTest::newRow("ok") << "00:1A:7D:DA:71:13" << true;
        QTest::newRow("lower") << "00:1a:7d:da:71:13" << false;
        QTest::newRow("short") << "00:1A:7D:DA:71" << false;
        QTest::newRow("long") << "00:1A:7D:DA:71:13:14" << false;
        QTest::newRow("dashes") << "00-1A-7D-DA-71-13" << false;
        QTest::newRow("empty") << "" << false;
        QTest::newRow("shell injection") << "00:1A:7D:DA:71:13; rm -rf ~" << false;
        QTest::newRow("newline") << "00:1A:7D:DA:71:13\nreboot" << false;
        QTest::newRow("subshell") << "$(reboot)" << false;
    }

    void addressValidation() {
        QFETCH(QString, address);
        QFETCH(bool, valid);
        QCOMPARE(isValidAddress(address), valid);
    }

    void devicePath() {
        QCOMPARE(devicePathFor(QStringLiteral("00:1A:7D:DA:71:13")), QStringLiteral("/org/bluez/hci0/dev_00_1A_7D_DA_71_13"));
    }

    void listOrderAndFiltering() {
        const QList<BluetoothEntry> all = {
            {QStringLiteral("00:00:00:00:00:01"), QStringLiteral("Nearby speaker")},
            {QStringLiteral("00:00:00:00:00:02"), QStringLiteral("00-00-00-00-00-02")},
            {QStringLiteral("00:00:00:00:00:03"), QStringLiteral("Paired car")},
            {QStringLiteral("00:00:00:00:00:04"), QStringLiteral("Connected phone")},
            {QStringLiteral("00:00:00:00:00:05"), QString()},
            {QStringLiteral("00:00:00:00:00:01"), QStringLiteral("Duplicate")},
        };
        const QSet<QString> paired = {QStringLiteral("00:00:00:00:00:03"), QStringLiteral("00:00:00:00:00:04"),
                                      QStringLiteral("00:00:00:00:00:05")};
        const QSet<QString> connected = {QStringLiteral("00:00:00:00:00:04")};
        const QVariantList list = buildDeviceList(all, paired, connected);

        QStringList names;
        for (const QVariant& item : list) {
            names << item.toMap().value(QStringLiteral("name")).toString();
        }
        QCOMPARE(names, (QStringList{"Connected phone", "Paired car", "00:00:00:00:00:05", "Nearby speaker"}));
        QVERIFY(list[0].toMap().value(QStringLiteral("connected")).toBool());
        QVERIFY(list[1].toMap().value(QStringLiteral("paired")).toBool());
        QVERIFY(!list[3].toMap().value(QStringLiteral("paired")).toBool());
    }

    void disabledDevicesDoNothing() {
        BluetoothDevices bluetooth(false);
        QSignalSpy devices(&bluetooth, &BluetoothDevices::devicesChanged);
        QSignalSpy state(&bluetooth, &BluetoothDevices::stateChanged);
        bluetooth.refresh();
        bluetooth.scan();
        bluetooth.connectDevice(QStringLiteral("00:1A:7D:DA:71:13"));
        bluetooth.disconnectDevice(QStringLiteral("00:1A:7D:DA:71:13"));
        bluetooth.forgetDevice(QStringLiteral("00:1A:7D:DA:71:13"));
        bluetooth.connectDevice(QStringLiteral("$(reboot)"));
        QCOMPARE(devices.count(), 0);
        QCOMPARE(state.count(), 0);
        QVERIFY(!bluetooth.scanning());
        QVERIFY(!bluetooth.busy());
        QVERIFY(bluetooth.devices().isEmpty());
        QVERIFY(!bluetooth.status().isEmpty());
    }
};

QTEST_GUILESS_MAIN(BluetoothListTest)
#include "tst_bluetooth_list.moc"
