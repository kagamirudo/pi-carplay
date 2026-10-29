#pragma once

#include "headunit/ApplicationManager.hpp"
#include "headunit/StorageManager.hpp"

#include <QObject>
#include <QString>
#include <QStringList>

class HeadunitController : public QObject {
  Q_OBJECT
  Q_PROPERTY(QString connectionSummary READ connectionSummary CONSTANT)
  Q_PROPERTY(QString linkMode READ linkMode CONSTANT)
  Q_PROPERTY(QString bluetoothSummary READ bluetoothSummary CONSTANT)
  Q_PROPERTY(QString dashcamSummary READ dashcamSummary CONSTANT)
  Q_PROPERTY(QString storageSummary READ storageSummary CONSTANT)
  Q_PROPERTY(QString displaySummary READ displaySummary CONSTANT)
  Q_PROPERTY(QStringList pages READ pages CONSTANT)

 public:
  explicit HeadunitController(QObject* parent = nullptr);

  QString connectionSummary() const;
  QString linkMode() const;
  QString bluetoothSummary() const;
  QString dashcamSummary() const;
  QString storageSummary() const;
  QString displaySummary() const;
  QStringList pages() const;

 private:
  headunit::ApplicationManager app_;
};
