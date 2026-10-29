#pragma once

#include "headunit/ConfigurationManager.hpp"
#include "headunit/stubs.hpp"

#include <string>

namespace headunit {

class ApplicationManager {
 public:
  explicit ApplicationManager(std::string configPath);

  const ConfigurationManager& config() const { return config_; }
  std::string connectionSummary() const;
  std::string dashcamSummary() const;
  int displayWidth() const;
  int displayHeight() const;

 private:
  ConfigurationManager config_;
  PhoneConnectionManager phones_;
  UsbManager usb_;
  WiFiManager wifi_;
  BluetoothManager bluetooth_;
  AudioRouter audio_;
  ProjectionManager projection_;
  AndroidAutoReceiver androidAuto_;
  CarPlayReceiver carPlay_;
  VideoRenderer video_;
  DashcamRecorder dashcam_;
  TouchInputManager touch_;
};

}  // namespace headunit
