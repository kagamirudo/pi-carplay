#include "connection/WifiDevices.hpp"

#include <QTest>
#include <QVariantMap>

using headunit::WifiDevices;

class WifiTest : public QObject {
    Q_OBJECT

private slots:
    void parsesKnownWifiProfilesOnly() {
        const QString show =
            "Wired connection 1:802-3-ethernet\n"
            "CreditGenie-Guest:802-11-wireless\n"
            "pi-carplay-aa:802-11-wireless\n"      // the hotspot is excluded
            "tailscale0:tun\n";
        const QStringList known = WifiDevices::parseKnownSsids(show);
        QCOMPARE(known.size(), 1);
        QVERIFY(known.contains("CreditGenie-Guest"));
        QVERIFY(!known.contains("pi-carplay-aa"));
    }

    void parsesNetworksWithActiveAndKnownFlags() {
        const QString list =
            "*:72:WPA2:CreditGenie-Guest\n"
            " :48:WPA2 WPA3:Quorum\n"
            " :30::OpenCafe\n";                     // empty security = open
        const auto nets = WifiDevices::parseNetworks(list, {"CreditGenie-Guest"});
        QCOMPARE(nets.size(), 3);
        const QVariantMap a = nets[0].toMap();
        QCOMPARE(a.value("ssid").toString(), QString("CreditGenie-Guest"));
        QCOMPARE(a.value("signal").toInt(), 72);
        QVERIFY(a.value("active").toBool());
        QVERIFY(a.value("known").toBool());
        const QVariantMap b = nets[1].toMap();
        QVERIFY(!b.value("active").toBool());
        QVERIFY(!b.value("known").toBool());
        QCOMPARE(nets[2].toMap().value("security").toString(), QString(""));
    }

    void skipsHiddenAndDedupes() {
        const QString list =
            "*:72:WPA2:Home\n"
            " :40:WPA2:Home\n"       // weaker duplicate dropped
            " :55:WPA2:\n";          // hidden (empty SSID) dropped
        const auto nets = WifiDevices::parseNetworks(list, {});
        QCOMPARE(nets.size(), 1);
        QCOMPARE(nets[0].toMap().value("signal").toInt(), 72);
    }

    void handlesEscapedColonInSsid() {
        const auto nets = WifiDevices::parseNetworks(" :60:WPA2:Cafe\\:2\n", {});
        QCOMPARE(nets.size(), 1);
        QCOMPARE(nets[0].toMap().value("ssid").toString(), QString("Cafe:2"));
    }
};

QTEST_MAIN(WifiTest)
#include "tst_wifi.moc"
