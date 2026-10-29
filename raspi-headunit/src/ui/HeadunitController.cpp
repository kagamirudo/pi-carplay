#include "HeadunitController.hpp"

#include <QCoreApplication>
#include <QFileInfo>

namespace {

QString configPath() {
  const QByteArray fromEnv = qgetenv("HEADUNIT_CONFIG");
  if (!fromEnv.isEmpty()) {
    return QString::fromLocal8Bit(fromEnv);
  }
  const QString dir = QCoreApplication::applicationDirPath();
  const QStringList candidates = {
      dir + QStringLiteral("/../config/headunit.conf"),
      dir + QStringLiteral("/config/headunit.conf"),
  };
  for (const QString& candidate : candidates) {
    if (QFileInfo::exists(candidate)) {
      return candidate;
    }
  }
  return candidates.front();
}

}  // namespace

HeadunitController::HeadunitController(QObject* parent)
    : QObject(parent), app_(configPath().toStdString()) {}

QString HeadunitController::connectionSummary() const {
  return QString::fromStdString(app_.connectionSummary());
}

QString HeadunitController::linkMode() const {
  return QString::fromStdString(app_.config().getString("link_mode", "wireless preferred, USB fallback"));
}

QString HeadunitController::bluetoothSummary() const {
  return QString::fromStdString(app_.config().getString("bluetooth_status", "Not paired to the car stereo"));
}

QString HeadunitController::dashcamSummary() const {
  return QString::fromStdString(app_.dashcamSummary());
}

QString HeadunitController::storageSummary() const {
  const auto capacity = static_cast<std::uint64_t>(
      app_.config().getInt("storage_capacity_mib", 0)) * 1024ull * 1024ull;
  const auto reserve = static_cast<std::uint64_t>(
      app_.config().getInt("storage_reserve_mib", 1024)) * 1024ull * 1024ull;
  const auto segment = static_cast<std::uint64_t>(
      app_.config().getInt("segment_mib", 150)) * 1024ull * 1024ull;
  const auto plan = headunit::planRetention(capacity, reserve, segment, {});
  if (capacity == 0) {
    return QStringLiteral("No footage disk configured");
  }
  return QStringLiteral("%1 clips fit on the footage disk")
      .arg(static_cast<qulonglong>(plan.maxSegments));
}

QString HeadunitController::displaySummary() const {
  return QStringLiteral("%1×%2  rotate %3")
      .arg(app_.displayWidth())
      .arg(app_.displayHeight())
      .arg(app_.config().getInt("rotation", 180));
}

QStringList HeadunitController::pages() const {
  return {QStringLiteral("Home"), QStringLiteral("Phone"),
          QStringLiteral("Dash cam"), QStringLiteral("Settings")};
}
