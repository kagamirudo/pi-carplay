#pragma once

#include <QList>
#include <QSet>
#include <QString>
#include <QVariantList>

namespace headunit {

struct BluetoothEntry {
    QString address;
    QString name;
};

// Lines of `bluetoothctl devices` output: "Device AA:BB:CC:DD:EE:FF Name".
QList<BluetoothEntry> parseDeviceList(const QString& output);

QSet<QString> addressesOf(const QList<BluetoothEntry>& entries);

// bluetoothctl names unnamed devices after their address with dashes.
bool isUnnamed(const BluetoothEntry& entry);

// Upper-case colon form only. Addresses end up in shell commands, so nothing
// else may pass.
bool isValidAddress(const QString& address);

QString devicePathFor(const QString& address);

// Connected first, then paired, then named nearby devices. Each entry is a map
// with address, name, paired and connected.
QVariantList buildDeviceList(const QList<BluetoothEntry>& all, const QSet<QString>& paired,
                             const QSet<QString>& connected);

}  // namespace headunit
