#pragma once

#include <string>

namespace headunit {

class WiFiManager {
public:
    struct Status {
        std::string interfaceName = "wlan0";
        bool exists = false;
        std::string operstate = "absent";
    };

    Status status() const;
};

}  // namespace headunit
