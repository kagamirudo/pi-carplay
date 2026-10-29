#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>

namespace headunit {

struct DisplayConfig {
    int width = 480;
    int height = 320;
    std::string output = "SPI-1";
    bool fullscreen = true;
    // Shell-only colour adjust (not Android Auto); QML scales these to -1..1.
    int brightness = 0;   // -100..100
    int contrast = 0;     // -100..100
    int hue = 0;          // -180..180
    int saturation = 0;   // -100..100
};

struct DashcamConfig {
    bool enabled = false;
    int width = 1920;
    int height = 1080;
    int fps = 30;
    int segmentSeconds = 120;
    std::uint64_t storageLimitBytes = 0;
    std::uint64_t reserveBytes = 512ull * 1024ull * 1024ull;
    std::string clipsDirectory;
};

struct LogConfig {
    std::string level = "info";
    std::string path;
};

struct BluetoothConfig {
    std::string carDevice;
};

struct AppConfig {
    DisplayConfig display;
    DashcamConfig dashcam;
    LogConfig logging;
    BluetoothConfig bluetooth;

    static AppConfig defaults();
};

struct ParseResult {
    bool ok = false;
    AppConfig config = AppConfig::defaults();
    std::string error;
};

class ConfigurationManager {
public:
    static constexpr std::uintmax_t kMaxFileBytes = 64 * 1024;

    static ParseResult parse(std::string_view text);
    static std::string serialize(const AppConfig& config);

    void reset(AppConfig config);
    const AppConfig& get() const;
    bool loadFile(const std::filesystem::path& path, std::string* error);
    bool saveFile(const std::filesystem::path& path, std::string* error) const;

private:
    AppConfig config_ = AppConfig::defaults();
};

}  // namespace headunit
