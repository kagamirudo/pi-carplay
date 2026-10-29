#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace headunit {

class USBManager {
public:
    struct Device {
        std::string sysfsName;
        std::string vendorId;
        std::string productId;
        std::string manufacturer;
        std::string product;
        bool hub = false;
    };

    explicit USBManager(std::filesystem::path root = "/sys/bus/usb/devices") : root_(std::move(root)) {}

    // Informational sysfs snapshot. This does not speak Android Auto or CarPlay.
    std::vector<Device> listDevices() const;

private:
    std::filesystem::path root_;
};

}  // namespace headunit
