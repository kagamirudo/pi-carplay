#include "connection/WiFiManager.hpp"

#include <fstream>

namespace headunit {

WiFiManager::Status WiFiManager::status() const {
    Status status;
    std::ifstream in(netRoot_ / status.interfaceName / "operstate");
    if (!in) {
        return status;
    }
    status.exists = true;
    std::getline(in, status.operstate);
    if (status.operstate.empty()) {
        status.operstate = "unknown";
    }
    return status;
}

}  // namespace headunit
