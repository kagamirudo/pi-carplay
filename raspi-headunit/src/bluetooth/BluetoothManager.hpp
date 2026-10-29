#pragma once

#include <filesystem>
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
    explicit BluetoothManager(std::string carDevice, std::filesystem::path rfkillRoot = "/sys/class/rfkill");
    StereoLink status() const;

private:
    std::string carDevice_;
    std::filesystem::path rfkillRoot_;
};

}  // namespace headunit
