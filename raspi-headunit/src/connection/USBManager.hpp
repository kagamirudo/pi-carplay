#pragma once

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

    // Informational sysfs snapshot. This does not speak Android Auto or CarPlay.
    std::vector<Device> listDevices() const;
};

}  // namespace headunit
