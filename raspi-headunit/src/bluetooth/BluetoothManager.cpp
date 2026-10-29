#include "bluetooth/BluetoothManager.hpp"

#include <filesystem>
#include <fstream>

namespace headunit {
namespace {

std::string readLine(const std::filesystem::path& path) {
    std::ifstream in(path);
    std::string line;
    std::getline(in, line);
    return line;
}

}  // namespace

BluetoothManager::BluetoothManager(std::string carDevice) : carDevice_(std::move(carDevice)) {}

StereoLink BluetoothManager::status() const {
    StereoLink link;
    link.carDevice = carDevice_;
    const std::filesystem::path root("/sys/class/rfkill");
    std::error_code ec;
    if (!std::filesystem::is_directory(root, ec)) {
        link.detail = "No rfkill devices.";
        return link;
    }
    for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(root, ec)) {
        if (ec) {
            break;
        }
        if (readLine(entry.path() / "type") != "bluetooth") {
            continue;
        }
        link.adapterPresent = true;
        link.softBlocked = readLine(entry.path() / "soft") != "0";
        if (link.softBlocked) {
            link.detail = "Adapter is soft-blocked. A2DP to the car is not started.";
        } else if (link.carDevice.empty()) {
            link.detail = "Adapter is on. No car stereo is named in the config.";
        } else {
            link.detail = "Adapter is on. Pairing to " + link.carDevice + " is not implemented.";
        }
        return link;
    }
    link.detail = "No Bluetooth adapter found.";
    return link;
}

}  // namespace headunit
