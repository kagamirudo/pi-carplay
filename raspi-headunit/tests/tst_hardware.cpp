// USB, Bluetooth rfkill and Wi-Fi probes, run against fake sysfs trees so the
// result does not depend on what the test machine has plugged in.
#include "bluetooth/BluetoothManager.hpp"
#include "connection/USBManager.hpp"
#include "connection/WiFiManager.hpp"

#include <QTemporaryDir>
#include <QTest>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <string>

using namespace headunit;
namespace fs = std::filesystem;

namespace {

void put(const fs::path& file, const std::string& text) {
    fs::create_directories(file.parent_path());
    std::ofstream(file, std::ios::binary) << text;
}

fs::path root(const QTemporaryDir& dir) { return fs::path(dir.path().toStdString()); }

}  // namespace

class HardwareTest : public QObject {
    Q_OBJECT

private slots:
    void usbListsDevicesWithAProductName() {
        QTemporaryDir dir;
        const fs::path usb = root(dir) / "usb";
        put(usb / "1-1" / "product", "Pixel 8\r\n");
        put(usb / "1-1" / "idVendor", "18d1\n");
        put(usb / "1-1" / "idProduct", "2d01 \n");
        put(usb / "1-1" / "manufacturer", "Google\n");
        put(usb / "1-1" / "bDeviceClass", "00\n");
        put(usb / "usb1" / "product", "xHCI Host Controller\n");
        put(usb / "usb1" / "bDeviceClass", "09\n");
        put(usb / "1-1:1.0" / "bInterfaceClass", "ff\n");
        put(usb / "stray-file", "not a device");

        auto devices = USBManager(usb).listDevices();
        std::sort(devices.begin(), devices.end(), [](const auto& a, const auto& b) { return a.sysfsName < b.sysfsName; });
        QCOMPARE(devices.size(), size_t(2));
        QCOMPARE(devices[0].sysfsName, std::string("1-1"));
        QCOMPARE(devices[0].product, std::string("Pixel 8"));
        QCOMPARE(devices[0].vendorId, std::string("18d1"));
        QCOMPARE(devices[0].productId, std::string("2d01"));
        QCOMPARE(devices[0].manufacturer, std::string("Google"));
        QVERIFY(!devices[0].hub);
        QVERIFY(devices[1].hub);
        QVERIFY(devices[1].vendorId.empty());
    }

    void usbWithoutSysfsIsEmpty() {
        QVERIFY(USBManager("/nonexistent/usb").listDevices().empty());
        QTemporaryDir dir;
        QVERIFY(USBManager(root(dir)).listDevices().empty());
    }

    void bluetoothWithoutRfkill() {
        const StereoLink link = BluetoothManager("Car", "/nonexistent/rfkill").status();
        QVERIFY(!link.adapterPresent);
        QCOMPARE(link.carDevice, std::string("Car"));
        QCOMPARE(link.detail, std::string("No rfkill devices."));
    }

    void bluetoothIgnoresOtherRadios() {
        QTemporaryDir dir;
        put(root(dir) / "rfkill0" / "type", "wlan\n");
        put(root(dir) / "rfkill0" / "soft", "0\n");
        const StereoLink link = BluetoothManager("", root(dir)).status();
        QVERIFY(!link.adapterPresent);
        QCOMPARE(link.detail, std::string("No Bluetooth adapter found."));
    }

    void bluetoothAdapterStates_data() {
        QTest::addColumn<QString>("soft");
        QTest::addColumn<QString>("car");
        QTest::addColumn<bool>("blocked");
        QTest::addColumn<QString>("detail");
        QTest::newRow("blocked") << "1" << "Car" << true << "Adapter is soft-blocked. A2DP to the car is not started.";
        QTest::newRow("unreadable") << "" << "Car" << true << "Adapter is soft-blocked. A2DP to the car is not started.";
        QTest::newRow("no car") << "0" << "" << false << "Adapter is on. No car stereo is named in the config.";
        QTest::newRow("car") << "0" << "Car Stereo" << false << "Adapter is on. Pairing to Car Stereo is not implemented.";
    }

    void bluetoothAdapterStates() {
        QFETCH(QString, soft);
        QFETCH(QString, car);
        QFETCH(bool, blocked);
        QFETCH(QString, detail);
        QTemporaryDir dir;
        put(root(dir) / "rfkill1" / "type", "bluetooth");
        if (!soft.isEmpty()) {
            put(root(dir) / "rfkill1" / "soft", soft.toStdString());
        }
        const StereoLink link = BluetoothManager(car.toStdString(), root(dir)).status();
        QVERIFY(link.adapterPresent);
        QCOMPARE(link.softBlocked, blocked);
        QCOMPARE(QString::fromStdString(link.detail), detail);
    }

    void wifiStates() {
        QCOMPARE(WiFiManager("/nonexistent/net").status().exists, false);
        QCOMPARE(WiFiManager("/nonexistent/net").status().operstate, std::string("absent"));

        QTemporaryDir dir;
        put(root(dir) / "wlan0" / "operstate", "up\n");
        WiFiManager::Status up = WiFiManager(root(dir)).status();
        QVERIFY(up.exists);
        QCOMPARE(up.operstate, std::string("up"));

        put(root(dir) / "wlan0" / "operstate", "");
        QCOMPARE(WiFiManager(root(dir)).status().operstate, std::string("unknown"));
    }
};

QTEST_GUILESS_MAIN(HardwareTest)
#include "tst_hardware.moc"
