#include "connection/PhoneConnectionManager.hpp"

#include <algorithm>
#include <cctype>
#include <string_view>

namespace headunit {
namespace {

std::string lower(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return text;
}

bool containsAndroid(std::string_view text) {
    const std::string folded = lower(std::string(text));
    return folded.find("android") != std::string::npos;
}

bool knownPhoneVendor(std::string_view vendor) {
    static constexpr std::string_view vendors[] = {
        "04e8",  // Samsung
        "18d1",  // Google
        "22b8",  // Motorola
        "0bb4",  // HTC
        "2717",  // Xiaomi
        "12d1",  // Huawei
        "0fce",  // Sony Ericsson
        "054c",  // Sony
        "1004",  // LG
        "2a70",  // OnePlus
        "22d9",  // OPPO
        "2d95",  // vivo
    };
    const std::string folded = lower(std::string(vendor));
    return std::find(std::begin(vendors), std::end(vendors), folded) != std::end(vendors);
}

bool accessoryProduct(std::string_view productId) {
    const std::string folded = lower(std::string(productId));
    return folded == "2d00" || folded == "2d01" || folded == "2d02" || folded == "2d03" || folded == "2d04" ||
           folded == "2d05";
}

}  // namespace

const char* transportName(Transport transport) {
    switch (transport) {
        case Transport::None:
            return "none";
        case Transport::Usb:
            return "usb";
        case Transport::Wireless:
            return "wireless";
    }
    return "none";
}

const char* linkStateName(LinkState state) {
    switch (state) {
        case LinkState::Idle:
            return "idle";
        case LinkState::Looking:
            return "looking";
        case LinkState::Unavailable:
            return "unavailable";
    }
    return "idle";
}

AndroidSighting findAndroidPhone(const std::vector<USBManager::Device>& devices) {
    AndroidSighting found;
    for (const USBManager::Device& device : devices) {
        if (device.hub || device.product.empty()) {
            continue;
        }
        const bool accessory = lower(device.vendorId) == "18d1" && accessoryProduct(device.productId);
        const bool android = accessory || containsAndroid(device.product) || containsAndroid(device.manufacturer) ||
                             knownPhoneVendor(device.vendorId);
        if (!android) {
            continue;
        }
        found.present = true;
        found.accessoryMode = accessory;
        found.name = device.product;
        if (accessory) {
            return found;
        }
    }
    return found;
}

const PhoneLink& PhoneConnectionManager::link() const { return link_; }

const AndroidSighting& PhoneConnectionManager::android() const { return android_; }

void PhoneConnectionManager::updateFromUsb(const std::vector<USBManager::Device>& devices) {
    android_ = findAndroidPhone(devices);
    if (!android_.present) {
        link_.transport = Transport::None;
        link_.state = LinkState::Idle;
        link_.summary = "No Android phone on USB.";
        return;
    }
    link_.transport = Transport::Usb;
    link_.state = LinkState::Looking;
    if (android_.accessoryMode) {
        link_.summary = "Android phone " + android_.name + " is in accessory mode. Video has not started.";
    } else {
        link_.summary = "Android phone " + android_.name + " is plugged in, still in storage mode.";
    }
}

void PhoneConnectionManager::lookForPhone() {
    if (!android_.present) {
        link_.state = LinkState::Looking;
        link_.summary = "Looking for an Android phone on USB.";
    }
}

}  // namespace headunit
