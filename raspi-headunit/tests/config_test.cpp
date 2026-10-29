#include "core/ConfigurationManager.hpp"
#include "test_harness.hpp"

#include <string>

namespace {

void testDefaults() {
    const auto parsed = headunit::ConfigurationManager::parse("{}");
    EXPECT(parsed.ok);
    EXPECT(parsed.config.display.width == 480);
    EXPECT(parsed.config.display.height == 320);
    EXPECT(parsed.config.display.output == "SPI-1");
    EXPECT(parsed.config.display.fullscreen);
    EXPECT(!parsed.config.dashcam.enabled);
    EXPECT(parsed.config.dashcam.width == 1920);
    EXPECT(parsed.config.dashcam.segmentSeconds == 120);
    EXPECT(parsed.config.dashcam.storageLimitBytes == 0);
    EXPECT(parsed.config.dashcam.reserveBytes == 512ull * 1024ull * 1024ull);
    EXPECT(parsed.config.logging.level == "info");
}

void testOverridesAndRoundTrip() {
    const char* text = R"({
      "display": {"width": 480, "height": 320, "output": "SPI-2", "fullscreen": false},
      "dashcam": {
        "enabled": false,
        "segmentSeconds": 60,
        "storageLimitBytes": 137438953472,
        "clipsDirectory": "/mnt/footage/clips \"a\""
      },
      "logging": {"level": "warning"},
      "bluetooth": {"carDevice": "Car Stereo"},
      "ignored": true
    })";
    const auto parsed = headunit::ConfigurationManager::parse(text);
    EXPECT(parsed.ok);
    if (!parsed.ok) {
        std::cerr << parsed.error << '\n';
    }
    EXPECT(!parsed.config.display.fullscreen);
    EXPECT(parsed.config.dashcam.segmentSeconds == 60);
    EXPECT(parsed.config.dashcam.width == 1920);
    EXPECT(parsed.config.dashcam.storageLimitBytes == 137438953472ull);
    EXPECT(parsed.config.dashcam.clipsDirectory == "/mnt/footage/clips \"a\"");
    EXPECT(parsed.config.logging.level == "warn");
    EXPECT(parsed.config.bluetooth.carDevice == "Car Stereo");

    const auto again = headunit::ConfigurationManager::parse(headunit::ConfigurationManager::serialize(parsed.config));
    EXPECT(again.ok);
    EXPECT(again.config.display.fullscreen == parsed.config.display.fullscreen);
    EXPECT(again.config.dashcam.clipsDirectory == parsed.config.dashcam.clipsDirectory);
    EXPECT(again.config.dashcam.storageLimitBytes == parsed.config.dashcam.storageLimitBytes);
    EXPECT(again.config.logging.level == "warn");
    EXPECT(again.config.bluetooth.carDevice == "Car Stereo");
}

void testRejects() {
    const auto type = headunit::ConfigurationManager::parse(R"({"display":{"width":"480"}})");
    EXPECT(!type.ok);
    EXPECT(type.error == "display.width must be a number");

    const auto negative = headunit::ConfigurationManager::parse(R"({"display":{"width":-1}})");
    EXPECT(!negative.ok);
    EXPECT(negative.error == "display.width must be positive");

    const auto junk = headunit::ConfigurationManager::parse("{}\n[]");
    EXPECT(!junk.ok);
    EXPECT(junk.error == "trailing data after JSON");

    const auto dup = headunit::ConfigurationManager::parse(R"({"display":{"width":480,"width":320}})");
    EXPECT(!dup.ok);
    EXPECT(dup.error == "duplicate key \"width\"");

    const auto level = headunit::ConfigurationManager::parse(R"({"logging":{"level":"loud"}})");
    EXPECT(!level.ok);
    EXPECT(level.error == "logging.level must be debug, info, warn, or error");

    const auto empty = headunit::ConfigurationManager::parse("   ");
    EXPECT(!empty.ok);
    EXPECT(empty.error == "empty config");

    const auto array = headunit::ConfigurationManager::parse("[]");
    EXPECT(!array.ok);
}

}  // namespace

void testConfig() {
    testDefaults();
    testOverridesAndRoundTrip();
    testRejects();
}
