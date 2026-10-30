#pragma once

#include "core/ApplicationManager.hpp"
#include "core/ShellPresenter.hpp"

#include <QObject>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQmlError>
#include <QQuickItem>
#include <QQuickWindow>
#include <QStringList>
#include <QTemporaryDir>
#include <QTest>
#include <QVariantList>
#include <QVariantMap>

#include <memory>

namespace headunit::test {

// Same QML surface as BluetoothDevices, without bluetoothctl or BlueZ.
class FakeBluetooth : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList devices READ devices NOTIFY devicesChanged)
    Q_PROPERTY(bool scanning READ scanning NOTIFY stateChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY stateChanged)
    Q_PROPERTY(QString status READ status NOTIFY stateChanged)

public:
    QVariantList devices() const { return devices_; }
    bool scanning() const { return scanning_; }
    bool busy() const { return busy_; }
    QString status() const { return status_; }

    void setDevices(QVariantList devices) {
        devices_ = std::move(devices);
        emit devicesChanged();
    }

    static QVariantMap device(const QString& address, const QString& name, bool paired, bool connected) {
        return {{QStringLiteral("address"), address},
                {QStringLiteral("name"), name},
                {QStringLiteral("paired"), paired},
                {QStringLiteral("connected"), connected}};
    }

    QStringList calls;

    Q_INVOKABLE void refresh() { calls << QStringLiteral("refresh"); }
    Q_INVOKABLE void scan() {
        calls << QStringLiteral("scan");
        scanning_ = !scanning_;
        status_ = scanning_ ? QStringLiteral("Scanning") : QStringLiteral("Idle");
        emit stateChanged();
    }
    Q_INVOKABLE void connectDevice(const QString& address) { record(QStringLiteral("connect ") + address); }
    Q_INVOKABLE void disconnectDevice(const QString& address) { record(QStringLiteral("disconnect ") + address); }
    Q_INVOKABLE void forgetDevice(const QString& address) { record(QStringLiteral("forget ") + address); }

signals:
    void devicesChanged();
    void stateChanged();

private:
    void record(const QString& call) {
        calls << call;
        busy_ = !busy_;
        emit stateChanged();
        busy_ = false;
        emit stateChanged();
    }

    QVariantList devices_;
    bool scanning_ = false;
    bool busy_ = false;
    QString status_ = QStringLiteral("Tap Scan");
};

// Same QML surface as WifiDevices, without nmcli.
class FakeWifi : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList networks READ networks NOTIFY networksChanged)
    Q_PROPERTY(bool scanning READ scanning NOTIFY stateChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY stateChanged)
    Q_PROPERTY(bool hotspotActive READ hotspotActive NOTIFY stateChanged)
    Q_PROPERTY(QString status READ status NOTIFY stateChanged)

public:
    QVariantList networks() const { return networks_; }
    bool scanning() const { return scanning_; }
    bool busy() const { return busy_; }
    bool hotspotActive() const { return hotspotActive_; }
    QString status() const { return status_; }

    void setNetworks(QVariantList n) {
        networks_ = std::move(n);
        emit networksChanged();
    }
    static QVariantMap network(const QString& ssid, int signal, const QString& security, bool active, bool known) {
        return {{QStringLiteral("ssid"), ssid},
                {QStringLiteral("signal"), signal},
                {QStringLiteral("security"), security},
                {QStringLiteral("active"), active},
                {QStringLiteral("known"), known}};
    }

    QStringList calls;

    Q_INVOKABLE void refresh() { calls << QStringLiteral("refresh"); }
    Q_INVOKABLE void scan() {
        calls << QStringLiteral("scan");
        scanning_ = !scanning_;
        emit stateChanged();
    }
    Q_INVOKABLE void connectNetwork(const QString& ssid) { calls << QStringLiteral("connect ") + ssid; }
    Q_INVOKABLE void forgetNetwork(const QString& ssid) { calls << QStringLiteral("forget ") + ssid; }
    Q_INVOKABLE void setHotspot(bool on) {
        hotspotActive_ = on;
        calls << (on ? QStringLiteral("hotspot on") : QStringLiteral("hotspot off"));
        emit stateChanged();
    }

signals:
    void networksChanged();
    void stateChanged();

private:
    QVariantList networks_;
    bool scanning_ = false;
    bool busy_ = false;
    bool hotspotActive_ = false;
    QString status_ = QStringLiteral("Tap Scan");
};

// The shell UI loaded from ui/ with the real presenter and a fake Bluetooth
// backend. Every QML warning is collected; tests fail on any.
class UiFixture {
public:
    UiFixture() : app_(AppConfig::defaults()), shell_(app_, savePath()) {
        resumeMarker_ = dir_.filePath(QStringLiteral("resumed"));
        shell_.setResumeCommand({QStringLiteral("/bin/sh"), QStringLiteral("-c"), QStringLiteral("touch \"$0\""), resumeMarker_});
        engine_.setOutputWarningsToStandardError(false);
        QObject::connect(&engine_, &QQmlEngine::warnings, &engine_, [this](const QList<QQmlError>& list) {
            for (const QQmlError& error : list) {
                warnings << error.toString();
            }
        });
        engine_.rootContext()->setContextProperty(QStringLiteral("shell"), &shell_);
        engine_.rootContext()->setContextProperty(QStringLiteral("bluetooth"), &bluetooth);
        engine_.rootContext()->setContextProperty(QStringLiteral("wifi"), &wifi);
        engine_.load(QUrl::fromLocalFile(QStringLiteral(HEADUNIT_SOURCE_DIR "/ui/Main.qml")));
    }

    std::filesystem::path savePath() const { return std::filesystem::path(dir_.path().toStdString()) / "headunit.json"; }
    QString resumeMarker() const { return resumeMarker_; }
    ShellPresenter& shell() { return shell_; }

    QQuickWindow* window() const {
        const auto roots = engine_.rootObjects();
        return roots.isEmpty() ? nullptr : qobject_cast<QQuickWindow*>(roots.first());
    }

    static bool isButton(const QObject* object) { return object->inherits("QQuickAbstractButton"); }

    static bool shown(const QQuickItem* item) {
        for (const QQuickItem* p = item; p; p = p->parentItem()) {
            if (!p->isVisible() || p->opacity() <= 0.0) {
                return false;
            }
        }
        return true;
    }

    // Every item in the visual tree, including the header and Repeater delegates.
    QList<QQuickItem*> items() const {
        QList<QQuickItem*> out;
        if (!window()) {
            return out;
        }
        QList<QQuickItem*> pending{window()->contentItem()};
        while (!pending.isEmpty()) {
            QQuickItem* item = pending.takeFirst();
            out << item;
            pending += item->childItems();
        }
        return out;
    }

    QList<QQuickItem*> buttons() const {
        QList<QQuickItem*> out;
        for (QQuickItem* item : items()) {
            if (isButton(item) && shown(item) && item->isEnabled()) {
                out << item;
            }
        }
        return out;
    }

    QQuickItem* button(const QString& text) const {
        for (QQuickItem* item : buttons()) {
            if (item->property("text").toString() == text) {
                return item;
            }
        }
        return nullptr;
    }

    // Scrolls the enclosing Flickable until the item is on screen, then taps its centre.
    bool tap(QQuickItem* item) {
        if (!item || !window()) {
            return false;
        }
        for (QQuickItem* p = item->parentItem(); p; p = p->parentItem()) {
            if (p->inherits("QQuickFlickable")) {
                const QPointF inFlick = item->mapToItem(p, QPointF(0, item->height() / 2));
                if (inFlick.y() < 0 || inFlick.y() > p->height()) {
                    const qreal contentY = p->property("contentY").toReal() + inFlick.y() - p->height() / 2;
                    const qreal maxY = std::max<qreal>(0, p->property("contentHeight").toReal() - p->height());
                    p->setProperty("contentY", std::clamp<qreal>(contentY, 0, maxY));
                }
                break;
            }
        }
        const QPointF centre = item->mapToScene(QPointF(item->width() / 2, item->height() / 2));
        if (!QRectF(0, 0, window()->width(), window()->height()).contains(centre)) {
            return false;
        }
        QTest::mouseClick(window(), Qt::LeftButton, {}, centre.toPoint());
        QCoreApplication::processEvents();
        return true;
    }

    QQuickItem* findByName(const QString& name) const {
        for (QQuickItem* item : items()) {
            if (item->objectName() == name) {
                return item;
            }
        }
        return nullptr;
    }

    QString currentPage() const {
        QQuickItem* nav = findByName(QStringLiteral("nav"));
        return nav ? nav->property("currentText").toString() : QString();
    }

    // Navigate via the dropdown page selector: set the StackLayout index.
    bool showPage(const QString& label) {
        QQuickItem* nav = findByName(QStringLiteral("nav"));
        QQuickItem* stack = findByName(QStringLiteral("stack"));
        if (!nav || !stack) {
            return false;
        }
        const int idx = nav->property("model").toStringList().indexOf(label);
        if (idx < 0) {
            return false;
        }
        stack->setProperty("currentIndex", idx);
        QCoreApplication::processEvents();
        return true;
    }

    FakeBluetooth bluetooth;
    FakeWifi wifi;
    QStringList warnings;

private:
    QTemporaryDir dir_;
    ApplicationManager app_;
    ShellPresenter shell_;
    QQmlApplicationEngine engine_;
    QString resumeMarker_;
};

}  // namespace headunit::test
