#include "connection/PhoneConnectionManager.hpp"
#include "test_harness.hpp"

namespace {

headunit::USBManager::Device device(std::string vendor, std::string productId, std::string product, bool hub = false) {
    headunit::USBManager::Device item;
    item.vendorId = std::move(vendor);
    item.productId = std::move(productId);
    item.product = std::move(product);
    item.hub = hub;
    return item;
}

void testGalaxyInStorageMode() {
    const headunit::AndroidSighting seen = headunit::findAndroidPhone(
        {device("04e8", "6860", "SAMSUNG_Android"), device("046d", "c53f", "USB Receiver")});
    EXPECT(seen.present);
    EXPECT(!seen.accessoryMode);
    EXPECT(seen.name == "SAMSUNG_Android");
}

void testAccessoryModeWins() {
    const headunit::AndroidSighting seen =
        headunit::findAndroidPhone({device("04e8", "6860", "SAMSUNG_Android"), device("18d1", "2d00", "Android")});
    EXPECT(seen.present);
    EXPECT(seen.accessoryMode);
    EXPECT(seen.name == "Android");
}

void testHubAndMouseAreIgnored() {
    headunit::USBManager::Device hub = device("1d6b", "0003", "xHCI Host Controller", true);
    const headunit::AndroidSighting seen =
        headunit::findAndroidPhone({hub, device("046d", "c53f", "USB Receiver")});
    EXPECT(!seen.present);
}

void testLinkFollowsThePhone() {
    headunit::PhoneConnectionManager phones;
    phones.lookForPhone();
    phones.updateFromUsb({device("04e8", "6860", "SAMSUNG_Android")});
    EXPECT(phones.link().transport == headunit::Transport::Usb);
    EXPECT(phones.link().summary.find("SAMSUNG_Android") != std::string::npos);
    EXPECT(phones.link().summary.find("not built") == std::string::npos);

    phones.updateFromUsb({});
    EXPECT(phones.link().summary == "No Android phone on USB.");
}

}  // namespace

void testPhoneConnection() {
    testGalaxyInStorageMode();
    testAccessoryModeWins();
    testHubAndMouseAreIgnored();
    testLinkFollowsThePhone();
}
