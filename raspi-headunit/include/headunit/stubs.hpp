#pragma once

#include <string>

namespace headunit {

// Boundaries from the architecture. None of these talk to a phone,
// the camera encoder, or Bluetooth yet.

class PhoneConnectionManager {
 public:
  std::string summary() const { return "No phone connected"; }
};

class UsbManager {
 public:
  std::string summary() const { return "USB idle"; }
};

class WiFiManager {
 public:
  std::string summary() const { return "Wi-Fi idle"; }
};

class BluetoothManager {
 public:
  std::string summary() const { return "Bluetooth audio not paired"; }
};

class AudioRouter {
 public:
  std::string summary() const { return "PipeWire route not set"; }
};

class ProjectionManager {
 public:
  std::string summary() const { return "Projection off"; }
};

class AndroidAutoReceiver {
 public:
  std::string summary() const { return "Android Auto not started"; }
};

class CarPlayReceiver {
 public:
  std::string summary() const { return "CarPlay not started"; }
};

class VideoRenderer {
 public:
  std::string summary() const { return "Phone video idle"; }
};

class DashcamRecorder {
 public:
  bool recording() const { return false; }
  std::string summary() const { return "Dash cam idle"; }
};

class TouchInputManager {
 public:
  std::string summary() const { return "ADS7846 via the desktop"; }
};

}  // namespace headunit
