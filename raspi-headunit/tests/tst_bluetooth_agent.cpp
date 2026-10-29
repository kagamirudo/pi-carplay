// Drives the pairing agent over a real D-Bus connection, the way bluetoothd
// calls it. Runs on a private session bus (dbus-run-session), never BlueZ.
#include "bluetooth/BluetoothAgent.hpp"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusObjectPath>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QSignalSpy>
#include <QTest>

using headunit::BluetoothAgent;

namespace {

const QString kTarget = QStringLiteral("/org/bluez/hci0/dev_00_1A_7D_DA_71_13");
const QString kOther = QStringLiteral("/org/bluez/hci0/dev_66_55_44_33_22_11");

}  // namespace

class BluetoothAgentTest : public QObject {
    Q_OBJECT

private:
    BluetoothAgent* agent_ = nullptr;
    QDBusConnection caller_{QStringLiteral("none")};

    // The agent runs on this thread, so make the blocking call from a second connection
    // while the event loop serves the first.
    QDBusMessage callAsync(const QString& method, const QVariantList& args) {
        QDBusMessage message = QDBusMessage::createMethodCall(QDBusConnection::sessionBus().baseService(),
                                                              QString::fromLatin1(BluetoothAgent::kPath),
                                                              QStringLiteral("org.bluez.Agent1"), method);
        message.setArguments(args);
        QDBusPendingCall pending = caller_.asyncCall(message, 5000);
        QDBusPendingCallWatcher watcher(pending);
        QSignalSpy done(&watcher, &QDBusPendingCallWatcher::finished);
        if (!watcher.isFinished()) {
            done.wait(5000);
        }
        return pending.reply();
    }

private slots:
    void initTestCase() {
        if (!QDBusConnection::sessionBus().isConnected()) {
            QSKIP("no session bus; run under dbus-run-session");
        }
        caller_ = QDBusConnection::connectToBus(QDBusConnection::SessionBus, QStringLiteral("caller"));
        QVERIFY(caller_.isConnected());
        agent_ = new BluetoothAgent(this);
        QVERIFY(agent_->exportOn(QDBusConnection::sessionBus()));
    }

    void rejectsWhenNoDeviceIsBeingPaired() {
        agent_->clearTarget();
        const QDBusMessage reply = callAsync(QStringLiteral("RequestPinCode"), {QVariant::fromValue(QDBusObjectPath(kTarget))});
        QCOMPARE(reply.type(), QDBusMessage::ErrorMessage);
        QCOMPARE(reply.errorName(), QStringLiteral("org.bluez.Error.Rejected"));
    }

    void answersPinForTheTarget_data() {
        QTest::addColumn<QString>("pin");
        QTest::newRow("0000") << "0000";
        QTest::newRow("1234") << "1234";
    }

    void answersPinForTheTarget() {
        QFETCH(QString, pin);
        agent_->setTarget(kTarget, pin);
        QSignalSpy messages(agent_, &BluetoothAgent::message);
        const QDBusMessage reply = callAsync(QStringLiteral("RequestPinCode"), {QVariant::fromValue(QDBusObjectPath(kTarget))});
        QCOMPARE(reply.type(), QDBusMessage::ReplyMessage);
        QCOMPARE(reply.arguments().value(0).toString(), pin);
        QCOMPARE(messages.count(), 1);
    }

    void passkeyIsThePinAsNumber() {
        agent_->setTarget(kTarget, QStringLiteral("1234"));
        const QDBusMessage reply = callAsync(QStringLiteral("RequestPasskey"), {QVariant::fromValue(QDBusObjectPath(kTarget))});
        QCOMPARE(reply.type(), QDBusMessage::ReplyMessage);
        QCOMPARE(reply.arguments().value(0).toUInt(), 1234u);
    }

    void rejectsOtherDevices_data() {
        QTest::addColumn<QString>("method");
        QTest::addColumn<QVariantList>("extra");
        QTest::newRow("pin") << "RequestPinCode" << QVariantList{};
        QTest::newRow("passkey") << "RequestPasskey" << QVariantList{};
        QTest::newRow("confirm") << "RequestConfirmation" << QVariantList{QVariant::fromValue(123456u)};
        QTest::newRow("authorize") << "RequestAuthorization" << QVariantList{};
        QTest::newRow("service") << "AuthorizeService" << QVariantList{QStringLiteral("0000110b-0000-1000-8000-00805f9b34fb")};
    }

    void rejectsOtherDevices() {
        QFETCH(QString, method);
        QFETCH(QVariantList, extra);
        agent_->setTarget(kTarget, QStringLiteral("0000"));
        const QDBusMessage reply = callAsync(method, QVariantList{QVariant::fromValue(QDBusObjectPath(kOther))} + extra);
        QCOMPARE(reply.type(), QDBusMessage::ErrorMessage);
        QCOMPARE(reply.errorName(), QStringLiteral("org.bluez.Error.Rejected"));
    }

    void acceptsTargetConfirmation() {
        agent_->setTarget(kTarget, QStringLiteral("0000"));
        const QDBusMessage reply = callAsync(QStringLiteral("RequestConfirmation"),
                                             {QVariant::fromValue(QDBusObjectPath(kTarget)), QVariant::fromValue(123456u)});
        QCOMPARE(reply.type(), QDBusMessage::ReplyMessage);
    }

    void releaseAndCancelAreAccepted() {
        QCOMPARE(callAsync(QStringLiteral("Cancel"), {}).type(), QDBusMessage::ReplyMessage);
        QCOMPARE(callAsync(QStringLiteral("Release"), {}).type(), QDBusMessage::ReplyMessage);
    }

    void directCallsOutsideDBusDoNotCrash() {
        agent_->clearTarget();
        QCOMPARE(agent_->RequestPinCode(QDBusObjectPath(kOther)), QString());
        QCOMPARE(agent_->RequestPasskey(QDBusObjectPath(kOther)), 0u);
        agent_->RequestAuthorization(QDBusObjectPath(kOther));
    }

    void cleanupTestCase() {
        QDBusConnection::sessionBus().unregisterObject(QString::fromLatin1(BluetoothAgent::kPath));
        QDBusConnection::disconnectFromBus(QStringLiteral("caller"));
    }
};

QTEST_GUILESS_MAIN(BluetoothAgentTest)
#include "tst_bluetooth_agent.moc"
