#include "bluetooth/BluetoothDevices.hpp"

#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusError>
#include <QDBusMessage>
#include <QDBusPendingCallWatcher>
#include <QRegularExpression>
#include <QSet>
#include <QVariantMap>

namespace headunit {
namespace {

const QRegularExpression kDeviceLine(QStringLiteral("^Device ([0-9A-F:]{17}) (.*)$"));

struct Entry {
    QString address;
    QString name;
};

QList<Entry> parseDevices(const QString& output) {
    QList<Entry> entries;
    for (const QString& line : output.split('\n', Qt::SkipEmptyParts)) {
        const QRegularExpressionMatch match = kDeviceLine.match(line.trimmed());
        if (match.hasMatch()) {
            entries.append({match.captured(1), match.captured(2).trimmed()});
        }
    }
    return entries;
}

QSet<QString> addresses(const QList<Entry>& entries) {
    QSet<QString> set;
    for (const Entry& entry : entries) {
        set.insert(entry.address);
    }
    return set;
}

// bluetoothctl names unnamed devices after their address with dashes.
bool unnamed(const Entry& entry) {
    QString dashed = entry.address;
    dashed.replace(':', '-');
    return entry.name.isEmpty() || entry.name == dashed;
}

}  // namespace

BluetoothDevices::BluetoothDevices(QObject* parent) : QObject(parent) {
    scanRefresh_.setInterval(3000);
    connect(&scanRefresh_, &QTimer::timeout, this, &BluetoothDevices::refresh);
    connect(&scanProcess_, &QProcess::finished, this, [this] {
        scanRefresh_.stop();
        scanning_ = false;
        refresh();
        setStatus(QStringLiteral("Scan finished. Tap a device to connect."));
    });
    connect(&actionProcess_, &QProcess::finished, this, [this](int exitCode) {
        busy_ = false;
        const QString error = QString::fromUtf8(actionProcess_.readAllStandardError()).trimmed();
        refresh();
        if (exitCode == 0) {
            setStatus(doneText_);
        } else {
            setStatus(error.isEmpty() ? QStringLiteral("Bluetooth action failed.") : error);
        }
    });
    connect(&agent_, &BluetoothAgent::message, this, &BluetoothDevices::setStatus);
    // bluetoothctl blocks waiting for bluetoothd when BlueZ is not running.
    QDBusConnectionInterface* bus = QDBusConnection::systemBus().interface();
    if (bus == nullptr || !bus->isServiceRegistered(QStringLiteral("org.bluez"))) {
        status_ = QStringLiteral("Bluetooth service is not running.");
        return;
    }
    refresh();
    const QString agentError = agent_.registerWithBluez();
    status_ = agentError.isEmpty() ? QStringLiteral("Tap Scan, then put the car or speaker in pairing mode.")
                                   : agentError;
}

bool BluetoothDevices::isPaired(const QString& address) const {
    for (const QVariant& value : devices_) {
        const QVariantMap device = value.toMap();
        if (device.value(QStringLiteral("address")).toString() == address) {
            return device.value(QStringLiteral("paired")).toBool();
        }
    }
    return false;
}

bool BluetoothDevices::validAddress(const QString& address) {
    static const QRegularExpression re(QStringLiteral("^[0-9A-F]{2}(:[0-9A-F]{2}){5}$"));
    return re.match(address).hasMatch();
}

QString BluetoothDevices::run(const QStringList& args, int timeoutMs) {
    QProcess process;
    process.start(QStringLiteral("bluetoothctl"), args);
    if (!process.waitForFinished(timeoutMs)) {
        process.kill();
        process.waitForFinished(500);
    }
    return QString::fromUtf8(process.readAllStandardOutput());
}

QString BluetoothDevices::nameOf(const QString& address) const {
    for (const QVariant& value : devices_) {
        const QVariantMap device = value.toMap();
        if (device.value(QStringLiteral("address")).toString() == address) {
            return device.value(QStringLiteral("name")).toString();
        }
    }
    return address;
}

void BluetoothDevices::setStatus(const QString& status) {
    status_ = status;
    emit stateChanged();
}

void BluetoothDevices::refresh() {
    const QList<Entry> all = parseDevices(run({QStringLiteral("devices")}));
    const QSet<QString> paired = addresses(parseDevices(run({QStringLiteral("devices"), QStringLiteral("Paired")})));
    const QSet<QString> connected =
        addresses(parseDevices(run({QStringLiteral("devices"), QStringLiteral("Connected")})));

    QVariantList connectedList;
    QVariantList pairedList;
    QVariantList nearbyList;
    for (const Entry& entry : all) {
        const bool isPaired = paired.contains(entry.address);
        const bool isConnected = connected.contains(entry.address);
        if (!isPaired && !isConnected && unnamed(entry)) {
            continue;
        }
        QVariantMap device;
        device.insert(QStringLiteral("address"), entry.address);
        device.insert(QStringLiteral("name"), entry.name);
        device.insert(QStringLiteral("paired"), isPaired);
        device.insert(QStringLiteral("connected"), isConnected);
        (isConnected ? connectedList : isPaired ? pairedList : nearbyList).append(device);
    }
    devices_ = connectedList + pairedList + nearbyList;
    emit devicesChanged();
}

void BluetoothDevices::scan() {
    if (scanning_) {
        return;
    }
    QProcess::execute(QStringLiteral("bluetoothctl"), {QStringLiteral("power"), QStringLiteral("on")});
    scanning_ = true;
    setStatus(QStringLiteral("Scanning for 25 s. Put the car or speaker in pairing mode."));
    scanProcess_.start(QStringLiteral("bluetoothctl"),
                       {QStringLiteral("--timeout"), QStringLiteral("25"), QStringLiteral("scan"), QStringLiteral("on")});
    scanRefresh_.start();
}

void BluetoothDevices::runAction(const QString& script, const QString& doing, const QString& done) {
    if (busy_) {
        return;
    }
    busy_ = true;
    doneText_ = done;
    setStatus(doing);
    actionProcess_.start(QStringLiteral("/bin/bash"), {QStringLiteral("-c"), script});
}

void BluetoothDevices::connectDevice(const QString& address) {
    if (!validAddress(address) || busy_) {
        return;
    }
    if (isPaired(address)) {
        connectPaired(address);
        return;
    }
    busy_ = true;
    pairWithPin(address, 0);
}

void BluetoothDevices::pairWithPin(const QString& address, int pinIndex) {
    const QString name = nameOf(address);
    const QString devicePath = QStringLiteral("/org/bluez/hci0/dev_") + QString(address).replace(':', '_');
    const QString pin = pins_.at(pinIndex);
    agent_.setTarget(devicePath, pin);
    setStatus(QStringLiteral("Pairing with %1 (PIN %2 if asked)…").arg(name, pin));

    const QDBusMessage call = QDBusMessage::createMethodCall(QStringLiteral("org.bluez"), devicePath,
                                                             QStringLiteral("org.bluez.Device1"), QStringLiteral("Pair"));
    auto* watcher = new QDBusPendingCallWatcher(QDBusConnection::systemBus().asyncCall(call, 60000), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, watcher, address, name, pinIndex] {
        watcher->deleteLater();
        const QDBusError error = watcher->error();
        if (!error.isValid() || error.name() == QStringLiteral("org.bluez.Error.AlreadyExists")) {
            agent_.clearTarget();
            busy_ = false;
            connectPaired(address);
            return;
        }
        const bool authFailed = error.name().startsWith(QStringLiteral("org.bluez.Error.Authentication"));
        if (authFailed && pinIndex + 1 < pins_.size()) {
            setStatus(QStringLiteral("PIN %1 refused, trying %2…").arg(pins_.at(pinIndex), pins_.at(pinIndex + 1)));
            QTimer::singleShot(1500, this, [this, address, pinIndex] { pairWithPin(address, pinIndex + 1); });
            return;
        }
        agent_.clearTarget();
        busy_ = false;
        refresh();
        if (error.name() == QStringLiteral("org.freedesktop.DBus.Error.UnknownObject")) {
            setStatus(QStringLiteral("%1 is out of range. Tap Scan and try again.").arg(name));
        } else if (authFailed) {
            setStatus(QStringLiteral("%1 refused PINs %2.").arg(name, pins_.join(QStringLiteral(" and "))));
        } else {
            setStatus(QStringLiteral("Pairing failed (%1). Is %2 in pairing mode?").arg(error.message(), name));
        }
    });
}

void BluetoothDevices::connectPaired(const QString& address) {
    const QString name = nameOf(address);
    // Trust so it reconnects on its own, connect, then make its A2DP sink the
    // default output and move playing streams to it.
    const QString script = QStringLiteral(
        "A=%1\n"
        "bluetoothctl trust $A >/dev/null\n"
        "for i in 1 2 3; do\n"
        "  bluetoothctl info $A | grep -q 'Connected: yes' && break\n"
        "  bluetoothctl --timeout 10 connect $A >/dev/null; sleep 2\n"
        "done\n"
        "bluetoothctl info $A | grep -q 'Connected: yes' || { echo 'Could not connect.' >&2; exit 1; }\n"
        "U=$(echo $A | tr : _)\n"
        "for i in $(seq 1 10); do\n"
        "  S=$(pactl list short sinks | awk -v u=$U '$2 ~ (\"bluez_output.\" u) {print $2; exit}')\n"
        "  [ -n \"$S\" ] && break; sleep 1\n"
        "done\n"
        "[ -n \"$S\" ] || { echo 'Connected, but no audio output appeared.' >&2; exit 1; }\n"
        "pactl set-default-sink $S\n"
        "for i in $(pactl list short sink-inputs | awk '{print $1}'); do pactl move-sink-input $i $S; done\n")
                               .arg(address);
    runAction(script, QStringLiteral("Connecting to %1…").arg(name),
              QStringLiteral("%1 connected. Sound now plays there.").arg(name));
}

void BluetoothDevices::disconnectDevice(const QString& address) {
    if (!validAddress(address)) {
        return;
    }
    const QString name = nameOf(address);
    runAction(QStringLiteral("bluetoothctl disconnect %1 >/dev/null").arg(address),
              QStringLiteral("Disconnecting %1…").arg(name), QStringLiteral("%1 disconnected.").arg(name));
}

void BluetoothDevices::forgetDevice(const QString& address) {
    if (!validAddress(address)) {
        return;
    }
    const QString name = nameOf(address);
    runAction(QStringLiteral("bluetoothctl remove %1 >/dev/null").arg(address),
              QStringLiteral("Forgetting %1…").arg(name), QStringLiteral("%1 removed.").arg(name));
}

}  // namespace headunit
