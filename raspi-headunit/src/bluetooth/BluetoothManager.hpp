#pragma once

#include <string>

namespace headunit {

struct StereoLink {
    bool adapterPresent = false;
    bool softBlocked = true;
    std::string carDevice;
    std::string profile = "A2DP source";
    std::string detail = "Bluetooth has not been probed.";
};

class BluetoothManager {
public:
    explicit BluetoothManager(std::string carDevice);
    StereoLink status() const;

private:
    std::string carDevice_;
};

}  // namespace headunit
