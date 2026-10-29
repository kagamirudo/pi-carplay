#pragma once

// JSON inputs for the config tests. Kept out of the test sources because moc
// cannot parse raw string literals there.

#include <string>
#include <vector>

namespace headunit::test {

inline const char* const kOverrides = R"({
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

struct Reject {
    const char* name;
    const char* text;
    const char* error;
};

inline const Reject kRejects[] = {
    {"string width", R"({"display":{"width":"480"}})", "display.width must be a number"},
    {"negative width", R"({"display":{"width":-1}})", "display.width must be positive"},
    {"trailing", "{}\n[]", "trailing data after JSON"},
    {"duplicate", R"({"display":{"width":480,"width":320}})", "duplicate key \"width\""},
    {"level", R"({"logging":{"level":"loud"}})", "logging.level must be debug, info, warn, or error"},
    {"empty", "   ", "empty config"},
    {"overflow", R"({"dashcam":{"storageLimitBytes":99999999999999999999999}})", "number overflow"},
    {"fraction", R"({"display":{"width":1.5}})", "fractional numbers are not used"},
    {"unterminated", R"({"bluetooth":{"carDevice":"abc)", "unterminated string"},
};

inline const std::vector<std::string> kFuzzSeeds = {
    "{}",
    R"({"display":{"width":480,"height":320,"output":"SPI-1","fullscreen":true}})",
    R"({"dashcam":{"enabled":false,"width":1920,"height":1080,"fps":30,"segmentSeconds":120,)"
    R"("storageLimitBytes":0,"reserveBytes":536870912,"clipsDirectory":"/mnt/footage/clips"}})",
    R"({"logging":{"level":"debug","path":"/tmp/h.log"},"bluetooth":{"carDevice":"Car \u00e9 \"x\""}})",
};

struct OddInput {
    const char* name;
    const char* text;
};

inline const OddInput kOddInputs[] = {
    {"lone surrogate", R"({"bluetooth":{"carDevice":"\ud800"}})"},
    {"bad escape at end", R"({"a":"\)"},
    {"short unicode", R"({"a":"\u12)"},
    {"max uint64", R"({"dashcam":{"storageLimitBytes":18446744073709551615}})"},
    {"int overflow fields", R"({"display":{"width":4294967297}})"},
    {"minus only", R"({"display":{"width":-}})"},
    {"invalid utf8", "{\"bluetooth\":{\"carDevice\":\"\xff\xfe\"}}"},
};

}  // namespace headunit::test
