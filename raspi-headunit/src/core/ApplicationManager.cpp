#include "core/ApplicationManager.hpp"

#include "core/Log.hpp"

namespace headunit {

ApplicationManager::ApplicationManager(AppConfig config)
    : bluetooth_(config.bluetooth.carDevice),
      dashcam_(config.dashcam),
      touch_(TouchMapping{}) {
    configuration_.reset(std::move(config));
    TouchMapping mapping;
    mapping.output = configuration_.get().display.output;
    touch_ = TouchInputManager(std::move(mapping));
    publishLogging();
    Logger::info("app", "head-unit shell ready");
}

const AppConfig& ApplicationManager::config() const { return configuration_.get(); }

void ApplicationManager::publishLogging() const {
    if (const auto level = parseLogLevel(configuration_.get().logging.level)) {
        Logger::setLevel(*level);
    }
    Logger::setPath(configuration_.get().logging.path);
}

bool ApplicationManager::setSegmentSeconds(int seconds) {
    if (seconds != 60 && seconds != 120 && seconds != 180) {
        return false;
    }
    AppConfig config = configuration_.get();
    config.dashcam.segmentSeconds = seconds;
    configuration_.reset(config);
    dashcam_.apply(config.dashcam);
    Logger::info("dashcam", "segment length updated");
    return true;
}

bool ApplicationManager::setLogLevel(const std::string& level) {
    const auto parsed = parseLogLevel(level);
    if (!parsed) {
        return false;
    }
    AppConfig config = configuration_.get();
    config.logging.level = logLevelName(*parsed);
    configuration_.reset(std::move(config));
    publishLogging();
    Logger::info("log", "level updated");
    return true;
}

bool ApplicationManager::setStorageLimitBytes(std::uint64_t bytes) {
    AppConfig config = configuration_.get();
    config.dashcam.storageLimitBytes = bytes;
    configuration_.reset(config);
    dashcam_.apply(config.dashcam);
    Logger::info("storage", "limit updated");
    return true;
}

bool ApplicationManager::setDisplayAdjust(int brightness, int contrast, int hue, int saturation) {
    const auto clamp = [](int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); };
    AppConfig config = configuration_.get();
    config.display.brightness = clamp(brightness, -100, 100);
    config.display.contrast = clamp(contrast, -100, 100);
    config.display.hue = clamp(hue, -180, 180);
    config.display.saturation = clamp(saturation, -100, 100);
    configuration_.reset(std::move(config));
    return true;
}

bool ApplicationManager::saveConfig(const std::filesystem::path& path, std::string* error) const {
    return configuration_.saveFile(path, error);
}

PhoneConnectionManager& ApplicationManager::phones() { return phones_; }

USBManager& ApplicationManager::usb() { return usb_; }

WiFiManager& ApplicationManager::wifi() { return wifi_; }

BluetoothManager& ApplicationManager::bluetooth() { return bluetooth_; }

AudioRouter& ApplicationManager::audio() { return audio_; }

ProjectionManager& ApplicationManager::projection() { return projection_; }

VideoRenderer& ApplicationManager::video() { return video_; }

DashcamRecorder& ApplicationManager::dashcam() { return dashcam_; }

StorageManager& ApplicationManager::storage() { return storage_; }

TouchInputManager& ApplicationManager::touch() { return touch_; }

}  // namespace headunit
