#pragma once

#include "connection/USBManager.hpp"

#include <string>
#include <vector>

namespace headunit {

enum class Transport { None, Usb, Wireless };
enum class LinkState { Idle, Looking, Unavailable };

struct PhoneLink {
    Transport transport = Transport::None;
    LinkState state = LinkState::Idle;
    std::string summary = "No phone session.";
};

struct AndroidSighting {
    bool present = false;
    bool accessoryMode = false;
    std::string name;
};

AndroidSighting findAndroidPhone(const std::vector<USBManager::Device>& devices);

class PhoneConnectionManager {
public:
    const PhoneLink& link() const;
    const AndroidSighting& android() const;
    void updateFromUsb(const std::vector<USBManager::Device>& devices);
    void lookForPhone();

private:
    PhoneLink link_;
    AndroidSighting android_;
};

const char* transportName(Transport transport);
const char* linkStateName(LinkState state);

}  // namespace headunit
