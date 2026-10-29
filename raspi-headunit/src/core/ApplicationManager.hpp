#pragma once

#include "audio/AudioRouter.hpp"
#include "bluetooth/BluetoothManager.hpp"
#include "camera/DashcamRecorder.hpp"
#include "camera/StorageManager.hpp"
#include "connection/PhoneConnectionManager.hpp"
#include "connection/USBManager.hpp"
#include "connection/WiFiManager.hpp"
#include "core/ConfigurationManager.hpp"
#include "input/TouchInputManager.hpp"
#include "projection/ProjectionManager.hpp"
#include "video/VideoRenderer.hpp"

#include <cstdint>
#include <filesystem>
#include <string>

namespace headunit {

class ApplicationManager {
public:
    explicit ApplicationManager(AppConfig config);

    const AppConfig& config() const;
    bool setSegmentSeconds(int seconds);
    bool setLogLevel(const std::string& level);
    bool setStorageLimitBytes(std::uint64_t bytes);
    bool saveConfig(const std::filesystem::path& path, std::string* error) const;

    PhoneConnectionManager& phones();
    USBManager& usb();
    WiFiManager& wifi();
    BluetoothManager& bluetooth();
    AudioRouter& audio();
    ProjectionManager& projection();
    VideoRenderer& video();
    DashcamRecorder& dashcam();
    StorageManager& storage();
    TouchInputManager& touch();

private:
    void publishLogging() const;

    ConfigurationManager configuration_;
    PhoneConnectionManager phones_;
    USBManager usb_;
    WiFiManager wifi_;
    BluetoothManager bluetooth_;
    AudioRouter audio_;
    ProjectionManager projection_;
    VideoRenderer video_;
    DashcamRecorder dashcam_;
    StorageManager storage_;
    TouchInputManager touch_;
};

}  // namespace headunit
