#include "connection/WifiDevices.hpp"

#include <QVariantMap>

namespace headunit {

namespace {
// nmcli -t escapes ':' and '\' in values as '\:' and '\\'. Undo that.
QString unescape(const QString& field) {
    QString out;
    for (int i = 0; i < field.size(); ++i) {
        if (field[i] == '\\' && i + 1 < field.size()) {
            out += field[++i];
        } else {
            out += field[i];
        }
    }
    return out;
}

// Split one nmcli -t line into n fields, respecting backslash-escaped colons.
// The last field keeps any remaining (unsplit) content.
QStringList splitFields(const QString& line, int n) {
    QStringList fields;
    QString cur;
    for (int i = 0; i < line.size(); ++i) {
        const QChar c = line[i];
        if (c == '\\' && i + 1 < line.size()) {
            cur += c;
            cur += line[++i];
        } else if (c == ':' && fields.size() < n - 1) {
            fields += cur;
            cur.clear();
        } else {
            cur += c;
        }
    }
    fields += cur;
    return fields;
}

// Shell-quote for a `nmcli ... '<arg>'` command.
QString sq(const QString& s) {
    QString e = s;
    e.replace(QStringLiteral("'"), QStringLiteral("'\\''"));
    return QStringLiteral("'") + e + QStringLiteral("'");
}
}  // namespace

QStringList WifiDevices::parseKnownSsids(const QString& connectionShow) {
    QStringList known;
    const QStringList lines = connectionShow.split('\n', Qt::SkipEmptyParts);
    for (const QString& line : lines) {
        const QStringList f = splitFields(line, 2);  // NAME:TYPE
        if (f.size() < 2) {
            continue;
        }
        const QString name = unescape(f[0]);
        if (f[1].contains(QStringLiteral("wireless")) && name != QLatin1String(kHotspot) && !name.isEmpty()) {
            known += name;
        }
    }
    return known;
}

QVariantList WifiDevices::parseNetworks(const QString& wifiList, const QStringList& knownSsids) {
    QVariantList out;
    QStringList seen;
    const QStringList lines = wifiList.split('\n', Qt::SkipEmptyParts);
    for (const QString& line : lines) {
        const QStringList f = splitFields(line, 4);  // IN-USE:SIGNAL:SECURITY:SSID
        if (f.size() < 4) {
            continue;
        }
        const QString ssid = unescape(f[3]).trimmed();
        if (ssid.isEmpty() || seen.contains(ssid)) {
            continue;  // hidden or duplicate (keep the first, which nmcli sorts strongest-first)
        }
        seen += ssid;
        QVariantMap net;
        net[QStringLiteral("ssid")] = ssid;
        net[QStringLiteral("signal")] = f[1].toInt();
        net[QStringLiteral("security")] = unescape(f[2]).trimmed();
        net[QStringLiteral("active")] = f[0].trimmed() == QLatin1String("*");
        net[QStringLiteral("known")] = knownSsids.contains(ssid);
        out += net;
    }
    return out;
}

WifiDevices::WifiDevices(bool enabled, QObject* parent) : QObject(parent), enabled_(enabled) {
    connect(&scanProcess_, &QProcess::finished, this, [this] {
        scanning_ = false;
        refresh();
        setStatus(QStringLiteral("Scan finished. Tap a saved network to connect."));
    });
    connect(&actionProcess_, &QProcess::finished, this, [this](int exitCode) {
        busy_ = false;
        const QString error = QString::fromUtf8(actionProcess_.readAllStandardError()).trimmed();
        refresh();
        setStatus(exitCode == 0 ? doneText_
                                : (error.isEmpty() ? QStringLiteral("Wi-Fi action failed.") : error));
    });
    if (!enabled_) {
        status_ = QStringLiteral("Wi-Fi is turned off for this session.");
        return;
    }
    if (run({QStringLiteral("-t"), QStringLiteral("-f"), QStringLiteral("RUNNING"), QStringLiteral("general")})
            .trimmed() != QLatin1String("running")) {
        enabled_ = false;
        status_ = QStringLiteral("NetworkManager is not running.");
        return;
    }
    refresh();
    status_ = QStringLiteral("Tap Scan to list networks.");
}

QString WifiDevices::run(const QStringList& args, int timeoutMs) {
    QProcess process;
    process.start(QStringLiteral("nmcli"), args);
    if (!process.waitForFinished(timeoutMs)) {
        process.kill();
        process.waitForFinished(500);
    }
    return QString::fromUtf8(process.readAllStandardOutput());
}

void WifiDevices::setStatus(const QString& status) {
    status_ = status;
    emit stateChanged();
}

bool WifiDevices::isKnown(const QString& ssid) const { return known_.contains(ssid); }

void WifiDevices::refresh() {
    if (!enabled_) {
        return;
    }
    known_ = parseKnownSsids(run({QStringLiteral("-t"), QStringLiteral("-f"), QStringLiteral("NAME,TYPE"),
                                  QStringLiteral("connection"), QStringLiteral("show")}));
    networks_ = parseNetworks(run({QStringLiteral("-t"), QStringLiteral("-f"),
                                   QStringLiteral("IN-USE,SIGNAL,SECURITY,SSID"), QStringLiteral("device"),
                                   QStringLiteral("wifi"), QStringLiteral("list")}),
                              known_);
    const QString active = run({QStringLiteral("-t"), QStringLiteral("-f"), QStringLiteral("NAME"),
                                QStringLiteral("connection"), QStringLiteral("show"), QStringLiteral("--active")});
    hotspotActive_ = active.split('\n').contains(QLatin1String(kHotspot));
    emit networksChanged();
    emit stateChanged();
}

void WifiDevices::scan() {
    if (!enabled_ || scanning_) {
        return;
    }
    scanning_ = true;
    setStatus(QStringLiteral("Scanning for networks…"));
    emit stateChanged();
    scanProcess_.start(QStringLiteral("nmcli"), {QStringLiteral("device"), QStringLiteral("wifi"),
                                                 QStringLiteral("list"), QStringLiteral("--rescan"),
                                                 QStringLiteral("yes")});
}

void WifiDevices::runAction(const QString& script, const QString& doing, const QString& done) {
    if (!enabled_ || busy_) {
        return;
    }
    busy_ = true;
    doneText_ = done;
    setStatus(doing);
    emit stateChanged();
    actionProcess_.start(QStringLiteral("/bin/bash"), {QStringLiteral("-c"), script});
}

void WifiDevices::connectNetwork(const QString& ssid) {
    if (!enabled_ || busy_ || ssid.isEmpty()) {
        return;
    }
    if (!isKnown(ssid)) {
        setStatus(QStringLiteral("%1 is not saved. Add it once with pi-carplay-wireless-setup (no keyboard here to "
                                 "type its password).")
                      .arg(ssid));
        return;
    }
    runAction(QStringLiteral("nmcli connection up ") + sq(ssid), QStringLiteral("Connecting to %1…").arg(ssid),
              QStringLiteral("Connected to %1.").arg(ssid));
}

void WifiDevices::forgetNetwork(const QString& ssid) {
    if (!enabled_ || busy_ || ssid.isEmpty()) {
        return;
    }
    runAction(QStringLiteral("nmcli connection delete ") + sq(ssid), QStringLiteral("Forgetting %1…").arg(ssid),
              QStringLiteral("Forgot %1.").arg(ssid));
}

void WifiDevices::setHotspot(bool on) {
    if (!enabled_ || busy_) {
        return;
    }
    if (on) {
        runAction(QStringLiteral("nmcli connection up ") + QLatin1String(kHotspot),
                  QStringLiteral("Starting hotspot…"),
                  QStringLiteral("Hotspot on. The Wi-Fi client link is dropped while it runs."));
    } else {
        runAction(QStringLiteral("nmcli connection down ") + QLatin1String(kHotspot),
                  QStringLiteral("Stopping hotspot…"), QStringLiteral("Hotspot off."));
    }
}

}  // namespace headunit
