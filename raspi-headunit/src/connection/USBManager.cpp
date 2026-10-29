#include "connection/USBManager.hpp"

#include <filesystem>
#include <fstream>

namespace headunit {
namespace {

std::string readTrimmed(const std::filesystem::path& path) {
    std::ifstream in(path);
    if (!in) {
        return {};
    }
    std::string line;
    std::getline(in, line);
    while (!line.empty() && (line.back() == '\n' || line.back() == '\r' || line.back() == ' ')) {
        line.pop_back();
    }
    return line;
}

}  // namespace

std::vector<USBManager::Device> USBManager::listDevices() const {
    std::vector<Device> devices;
    const std::filesystem::path root("/sys/bus/usb/devices");
    std::error_code ec;
    if (!std::filesystem::is_directory(root, ec)) {
        return devices;
    }
    for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(root, ec)) {
        if (ec || !entry.is_directory()) {
            continue;
        }
        const std::string product = readTrimmed(entry.path() / "product");
        if (product.empty()) {
            continue;
        }
        Device device;
        device.sysfsName = entry.path().filename().string();
        device.vendorId = readTrimmed(entry.path() / "idVendor");
        device.productId = readTrimmed(entry.path() / "idProduct");
        device.manufacturer = readTrimmed(entry.path() / "manufacturer");
        device.product = product;
        device.hub = readTrimmed(entry.path() / "bDeviceClass") == "09";
        devices.push_back(std::move(device));
    }
    return devices;
}

}  // namespace headunit
