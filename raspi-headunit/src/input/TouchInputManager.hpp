#pragma once

#include <string>

namespace headunit {

struct TouchMapping {
    std::string deviceName = "ADS7846 Touchscreen";
    std::string output = "SPI-1";
    bool mouseEmulation = true;
    bool forwardingToPhone = false;
};

class TouchInputManager {
public:
    explicit TouchInputManager(TouchMapping mapping);
    const TouchMapping& mapping() const;
    // Reads the event node name. Does not grab the device.
    std::string eventNode() const;

private:
    TouchMapping mapping_;
};

}  // namespace headunit
