#pragma once

#include "core/ApplicationManager.hpp"

#include <QObject>
#include <QString>
#include <QStringList>
#include <filesystem>

namespace headunit {

class ShellPresenter : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString homePhone READ homePhone NOTIFY changed)
    Q_PROPERTY(QString homeCamera READ homeCamera NOTIFY changed)
    Q_PROPERTY(QString homeDisplay READ homeDisplay NOTIFY changed)
    Q_PROPERTY(QString linkState READ linkState NOTIFY changed)
    Q_PROPERTY(QString linkDetail READ linkDetail NOTIFY changed)
    Q_PROPERTY(QString usbSummary READ usbSummary NOTIFY changed)
    Q_PROPERTY(QString wifiSummary READ wifiSummary NOTIFY changed)
    Q_PROPERTY(QString bluetoothSummary READ bluetoothSummary NOTIFY changed)
    Q_PROPERTY(QString audioSummary READ audioSummary NOTIFY changed)
    Q_PROPERTY(QString androidAuto READ androidAuto NOTIFY changed)
    Q_PROPERTY(QString carPlay READ carPlay NOTIFY changed)
    Q_PROPERTY(QString screenOwner READ screenOwner NOTIFY changed)
    Q_PROPERTY(QString videoSummary READ videoSummary NOTIFY changed)
    Q_PROPERTY(QString recordState READ recordState NOTIFY changed)
    Q_PROPERTY(QString recordDetail READ recordDetail NOTIFY changed)
    Q_PROPERTY(QString recordGeometry READ recordGeometry NOTIFY changed)
    Q_PROPERTY(QString focusSummary READ focusSummary NOTIFY changed)
    Q_PROPERTY(QString lastClip READ lastClip NOTIFY changed)
    Q_PROPERTY(QString storageSummary READ storageSummary NOTIFY changed)
    Q_PROPERTY(QString segmentLabel READ segmentLabel NOTIFY changed)
    Q_PROPERTY(QString logLevel READ logLevel NOTIFY changed)
    Q_PROPERTY(QString storageLimitLabel READ storageLimitLabel NOTIFY changed)
    Q_PROPERTY(QString displaySummary READ displaySummary NOTIFY changed)
    Q_PROPERTY(QString touchSummary READ touchSummary NOTIFY changed)
    Q_PROPERTY(QString saveResult READ saveResult NOTIFY changed)
    Q_PROPERTY(bool previewOpen READ previewOpen NOTIFY changed)
    Q_PROPERTY(QString version READ version CONSTANT)

public:
    ShellPresenter(ApplicationManager& app, std::filesystem::path savePath, QObject* parent = nullptr);

    QString homePhone() const { return homePhone_; }
    QString homeCamera() const { return homeCamera_; }
    QString homeDisplay() const { return homeDisplay_; }
    QString linkState() const { return linkState_; }
    QString linkDetail() const { return linkDetail_; }
    QString usbSummary() const { return usbSummary_; }
    QString wifiSummary() const { return wifiSummary_; }
    QString bluetoothSummary() const { return bluetoothSummary_; }
    QString audioSummary() const { return audioSummary_; }
    QString androidAuto() const { return androidAuto_; }
    QString carPlay() const { return carPlay_; }
    QString screenOwner() const { return screenOwner_; }
    QString videoSummary() const { return videoSummary_; }
    QString recordState() const { return recordState_; }
    QString recordDetail() const { return recordDetail_; }
    QString recordGeometry() const { return recordGeometry_; }
    QString focusSummary() const { return focusSummary_; }
    QString lastClip() const { return lastClip_; }
    QString storageSummary() const { return storageSummary_; }
    QString segmentLabel() const { return segmentLabel_; }
    QString logLevel() const { return logLevel_; }
    QString storageLimitLabel() const { return storageLimitLabel_; }
    QString displaySummary() const { return displaySummary_; }
    QString touchSummary() const { return touchSummary_; }
    QString saveResult() const { return saveResult_; }
    bool previewOpen() const { return previewOpen_; }
    QString version() const;

    Q_INVOKABLE void lookForPhone();
    Q_INVOKABLE void refresh();
    Q_INVOKABLE void cycleSegment();
    Q_INVOKABLE void cycleLogLevel();
    Q_INVOKABLE void adjustStorageLimit(int deltaSteps);
    Q_INVOKABLE void saveSettings();
    Q_INVOKABLE void requestRecord();
    Q_INVOKABLE void togglePreview();
    Q_INVOKABLE void openAndroidAuto();

    // Program and arguments run by openAndroidAuto(). Defaults to signalling autoapp.
    void setResumeCommand(QStringList command);
    // Shown in place of the save result, e.g. when settings were reset at start-up.
    void setNotice(const QString& text);

signals:
    void changed();

private:
    QString formatBytes(std::uint64_t bytes) const;
    std::filesystem::path previewFlagPath() const;
    bool readPreviewFlag() const;

    ApplicationManager& app_;
    std::filesystem::path savePath_;
    QString homePhone_;
    QString homeCamera_;
    QString homeDisplay_;
    QString linkState_;
    QString linkDetail_;
    QString usbSummary_;
    QString wifiSummary_;
    QString bluetoothSummary_;
    QString audioSummary_;
    QString androidAuto_;
    QString carPlay_;
    QString screenOwner_;
    QString videoSummary_;
    QString recordState_;
    QString recordDetail_;
    QString recordGeometry_;
    QString focusSummary_;
    QString lastClip_;
    QString storageSummary_;
    QString segmentLabel_;
    QString logLevel_;
    QString storageLimitLabel_;
    QString displaySummary_;
    QString touchSummary_;
    QString saveResult_ = "Not saved yet";
    bool previewOpen_ = false;
    // OpenAuto shows its projection window again on SIGUSR1.
    QStringList resumeCommand_{QStringLiteral("pkill"), QStringLiteral("-USR1"), QStringLiteral("-x"),
                               QStringLiteral("autoapp")};
};

}  // namespace headunit
