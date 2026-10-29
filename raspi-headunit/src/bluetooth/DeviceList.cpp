#include "bluetooth/DeviceList.hpp"

#include <QRegularExpression>
#include <QVariantMap>

namespace headunit {

QList<BluetoothEntry> parseDeviceList(const QString& output) {
    static const QRegularExpression line(QStringLiteral("^Device ([0-9A-F]{2}(?::[0-9A-F]{2}){5}) ?(.*)$"));
    QList<BluetoothEntry> entries;
    for (const QString& text : output.split('\n', Qt::SkipEmptyParts)) {
        const QRegularExpressionMatch match = line.match(text.trimmed());
        if (match.hasMatch()) {
            entries.append({match.captured(1), match.captured(2).trimmed()});
        }
    }
    return entries;
}

QSet<QString> addressesOf(const QList<BluetoothEntry>& entries) {
    QSet<QString> set;
    for (const BluetoothEntry& entry : entries) {
        set.insert(entry.address);
    }
    return set;
}

bool isUnnamed(const BluetoothEntry& entry) {
    QString dashed = entry.address;
    dashed.replace(':', '-');
    return entry.name.isEmpty() || entry.name == dashed;
}

bool isValidAddress(const QString& address) {
    static const QRegularExpression re(QStringLiteral("^[0-9A-F]{2}(:[0-9A-F]{2}){5}$"));
    return re.match(address).hasMatch();
}

QString devicePathFor(const QString& address) {
    return QStringLiteral("/org/bluez/hci0/dev_") + QString(address).replace(':', '_');
}

QVariantList buildDeviceList(const QList<BluetoothEntry>& all, const QSet<QString>& paired,
                             const QSet<QString>& connected) {
    QVariantList connectedList;
    QVariantList pairedList;
    QVariantList nearbyList;
    QSet<QString> seen;
    for (const BluetoothEntry& entry : all) {
        if (seen.contains(entry.address)) {
            continue;
        }
        seen.insert(entry.address);
        const bool isPaired = paired.contains(entry.address);
        const bool isConnected = connected.contains(entry.address);
        if (!isPaired && !isConnected && isUnnamed(entry)) {
            continue;
        }
        QVariantMap device;
        device.insert(QStringLiteral("address"), entry.address);
        device.insert(QStringLiteral("name"), entry.name.isEmpty() ? entry.address : entry.name);
        device.insert(QStringLiteral("paired"), isPaired);
        device.insert(QStringLiteral("connected"), isConnected);
        (isConnected ? connectedList : isPaired ? pairedList : nearbyList).append(device);
    }
    return connectedList + pairedList + nearbyList;
}

}  // namespace headunit
