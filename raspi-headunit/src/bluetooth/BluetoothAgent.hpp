#pragma once

#include <QDBusConnection>
#include <QDBusContext>
#include <QDBusObjectPath>
#include <QObject>
#include <QString>

namespace headunit {

// org.bluez.Agent1 that answers pairing requests for the one device the user
// chose to connect: legacy PIN requests get the current PIN, numeric
// comparison is accepted. Requests from any other device are rejected.
class BluetoothAgent : public QObject, protected QDBusContext {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.bluez.Agent1")

public:
    static constexpr const char* kPath = "/headunit/bluetooth/agent";

    explicit BluetoothAgent(QObject* parent = nullptr);

    // Registers with BlueZ as the default agent. Returns an error text, or empty on success.
    QString registerWithBluez();
    // Publishes the agent object at kPath on a bus without registering with BlueZ.
    bool exportOn(QDBusConnection bus);
    void setTarget(const QString& devicePath, const QString& pin);
    void clearTarget();

public slots:
    void Release();
    QString RequestPinCode(const QDBusObjectPath& device);
    void DisplayPinCode(const QDBusObjectPath& device, const QString& pincode);
    uint RequestPasskey(const QDBusObjectPath& device);
    void DisplayPasskey(const QDBusObjectPath& device, uint passkey, ushort entered);
    void RequestConfirmation(const QDBusObjectPath& device, uint passkey);
    void RequestAuthorization(const QDBusObjectPath& device);
    void AuthorizeService(const QDBusObjectPath& device, const QString& uuid);
    void Cancel();

signals:
    void message(const QString& text);

private:
    bool isTarget(const QDBusObjectPath& device);

    QString targetPath_;
    QString pin_;
};

}  // namespace headunit
