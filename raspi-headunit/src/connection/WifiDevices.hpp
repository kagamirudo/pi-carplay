#pragma once

#include <QMap>
#include <QObject>
#include <QProcess>
#include <QString>
#include <QStringList>
#include <QVariantList>

namespace headunit {

// Scans Wi-Fi, connects to already-saved networks, forgets them, and toggles the
// wireless-Android-Auto hotspot, all through nmcli. It never types a password
// (the head unit has no keyboard): joining a brand-new secured network is still
// done once with pi-carplay-wireless-setup. With enabled=false every action is a no-op.
class WifiDevices : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList networks READ networks NOTIFY networksChanged)
    Q_PROPERTY(bool scanning READ scanning NOTIFY stateChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY stateChanged)
    Q_PROPERTY(bool hotspotActive READ hotspotActive NOTIFY stateChanged)
    Q_PROPERTY(QString status READ status NOTIFY stateChanged)

public:
    explicit WifiDevices(bool enabled = true, QObject* parent = nullptr);

    QVariantList networks() const { return networks_; }
    bool scanning() const { return scanning_; }
    bool busy() const { return busy_; }
    bool hotspotActive() const { return hotspotActive_; }
    QString status() const { return status_; }

    Q_INVOKABLE void refresh();
    Q_INVOKABLE void scan();
    Q_INVOKABLE void connectNetwork(const QString& ssid);  // saved networks only
    Q_INVOKABLE void forgetNetwork(const QString& ssid);
    Q_INVOKABLE void setHotspot(bool on);

    // Pure parsers, for tests. parseNetworks reads `nmcli -t -f IN-USE,SIGNAL,SECURITY,SSID
    // device wifi list`; parseKnownSsids reads `nmcli -t -f NAME,TYPE connection show`.
    static QVariantList parseNetworks(const QString& wifiList, const QStringList& knownSsids);
    static QStringList parseKnownSsids(const QString& connectionShow);
    static constexpr const char* kHotspot = "pi-carplay-aa";

signals:
    void networksChanged();
    void stateChanged();

private:
    static QString run(const QStringList& args, int timeoutMs = 5000);
    void runAction(const QString& script, const QString& doing, const QString& done);
    void setStatus(const QString& status);
    bool isKnown(const QString& ssid) const;

    QVariantList networks_;
    QMap<QString, QString> knownBySsid_;  // SSID -> NetworkManager profile name
    QProcess scanProcess_;
    QProcess actionProcess_;
    bool enabled_ = true;
    bool scanning_ = false;
    bool busy_ = false;
    bool hotspotActive_ = false;
    QString status_;
    QString doneText_;
};

}  // namespace headunit
