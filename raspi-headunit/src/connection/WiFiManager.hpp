#pragma once

#include <filesystem>
#include <string>

namespace headunit {

class WiFiManager {
public:
    struct Status {
        std::string interfaceName = "wlan0";
        bool exists = false;
        std::string operstate = "absent";
    };

    explicit WiFiManager(std::filesystem::path netRoot = "/sys/class/net") : netRoot_(std::move(netRoot)) {}

    Status status() const;

private:
    std::filesystem::path netRoot_;
};

}  // namespace headunit
