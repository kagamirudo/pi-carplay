#include "bluetooth/BluetoothAgent.hpp"

#include <QDBusConnection>
#include <QDBusMessage>

namespace headunit {

BluetoothAgent::BluetoothAgent(QObject* parent) : QObject(parent) {}

QString BluetoothAgent::registerWithBluez() {
    QDBusConnection bus = QDBusConnection::systemBus();
    if (!bus.isConnected()) {
        return QStringLiteral("No system D-Bus.");
    }
    if (!exportOn(bus)) {
        return QStringLiteral("Could not export the pairing agent.");
    }
    const QDBusObjectPath path(QString::fromLatin1(kPath));
    for (const auto& [method, args] : {std::pair{QStringLiteral("RegisterAgent"),
                                                 QVariantList{QVariant::fromValue(path), QStringLiteral("KeyboardDisplay")}},
                                       std::pair{QStringLiteral("RequestDefaultAgent"),
                                                 QVariantList{QVariant::fromValue(path)}}}) {
        QDBusMessage call = QDBusMessage::createMethodCall(QStringLiteral("org.bluez"), QStringLiteral("/org/bluez"),
                                                           QStringLiteral("org.bluez.AgentManager1"), method);
        call.setArguments(args);
        const QDBusMessage reply = bus.call(call, QDBus::Block, 3000);
        if (reply.type() == QDBusMessage::ErrorMessage) {
            return QStringLiteral("Pairing agent: %1").arg(reply.errorMessage());
        }
    }
    return {};
}

bool BluetoothAgent::exportOn(QDBusConnection bus) {
    return bus.isConnected() && bus.registerObject(QString::fromLatin1(kPath), this, QDBusConnection::ExportAllSlots);
}

void BluetoothAgent::setTarget(const QString& devicePath, const QString& pin) {
    targetPath_ = devicePath;
    pin_ = pin;
}

void BluetoothAgent::clearTarget() {
    targetPath_.clear();
    pin_.clear();
}

bool BluetoothAgent::isTarget(const QDBusObjectPath& device) {
    if (!targetPath_.isEmpty() && device.path() == targetPath_) {
        return true;
    }
    if (calledFromDBus()) {
        sendErrorReply(QStringLiteral("org.bluez.Error.Rejected"), QStringLiteral("Not the device being paired"));
    }
    return false;
}

void BluetoothAgent::Release() {}

QString BluetoothAgent::RequestPinCode(const QDBusObjectPath& device) {
    if (!isTarget(device)) {
        return {};
    }
    emit message(QStringLiteral("Sending PIN %1…").arg(pin_));
    return pin_;
}

void BluetoothAgent::DisplayPinCode(const QDBusObjectPath&, const QString& pincode) {
    emit message(QStringLiteral("Enter PIN %1 on the car.").arg(pincode));
}

uint BluetoothAgent::RequestPasskey(const QDBusObjectPath& device) {
    if (!isTarget(device)) {
        return 0;
    }
    emit message(QStringLiteral("Sending passkey %1…").arg(pin_));
    return pin_.toUInt();
}

void BluetoothAgent::DisplayPasskey(const QDBusObjectPath&, uint passkey, ushort) {
    emit message(QStringLiteral("Enter %1 on the car.").arg(passkey, 6, 10, QLatin1Char('0')));
}

void BluetoothAgent::RequestConfirmation(const QDBusObjectPath& device, uint passkey) {
    if (isTarget(device)) {
        emit message(QStringLiteral("If the car shows %1, confirm it there.").arg(passkey, 6, 10, QLatin1Char('0')));
    }
}

void BluetoothAgent::RequestAuthorization(const QDBusObjectPath& device) {
    isTarget(device);
}

void BluetoothAgent::AuthorizeService(const QDBusObjectPath& device, const QString&) {
    isTarget(device);
}

void BluetoothAgent::Cancel() {}

}  // namespace headunit
