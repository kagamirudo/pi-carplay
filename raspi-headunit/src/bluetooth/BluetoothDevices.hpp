#pragma once

#include "bluetooth/BluetoothAgent.hpp"

#include <QObject>
#include <QProcess>
#include <QString>
#include <QTimer>
#include <QVariantList>

namespace headunit {

// Lists nearby and paired Bluetooth devices through bluetoothctl and connects
// one as the audio output (car stereo or speaker, A2DP sink).
class BluetoothDevices : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList devices READ devices NOTIFY devicesChanged)
    Q_PROPERTY(bool scanning READ scanning NOTIFY stateChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY stateChanged)
    Q_PROPERTY(QString status READ status NOTIFY stateChanged)

public:
    explicit BluetoothDevices(QObject* parent = nullptr);

    QVariantList devices() const { return devices_; }
    bool scanning() const { return scanning_; }
    bool busy() const { return busy_; }
    QString status() const { return status_; }

    Q_INVOKABLE void refresh();
    Q_INVOKABLE void scan();
    Q_INVOKABLE void connectDevice(const QString& address);
    Q_INVOKABLE void disconnectDevice(const QString& address);
    Q_INVOKABLE void forgetDevice(const QString& address);

signals:
    void devicesChanged();
    void stateChanged();

private:
    static bool validAddress(const QString& address);
    static QString run(const QStringList& args, int timeoutMs = 4000);
    void runAction(const QString& script, const QString& doing, const QString& done);
    void setStatus(const QString& status);
    QString nameOf(const QString& address) const;
    bool isPaired(const QString& address) const;
    void pairWithPin(const QString& address, int pinIndex);
    void connectPaired(const QString& address);

    // Car stereos with legacy pairing usually take one of these.
    const QStringList pins_{QStringLiteral("0000"), QStringLiteral("1234")};
    BluetoothAgent agent_;
    QVariantList devices_;
    QProcess scanProcess_;
    QProcess actionProcess_;
    QTimer scanRefresh_;
    bool scanning_ = false;
    bool busy_ = false;
    QString status_;
    QString doneText_;
};

}  // namespace headunit
