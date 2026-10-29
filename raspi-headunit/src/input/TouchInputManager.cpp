#include "input/TouchInputManager.hpp"

#include <filesystem>
#include <fstream>

namespace headunit {
namespace {

std::string readLine(const std::filesystem::path& path) {
    std::ifstream in(path);
    std::string line;
    std::getline(in, line);
    while (!line.empty() && (line.back() == '\n' || line.back() == '\r')) {
        line.pop_back();
    }
    return line;
}

}  // namespace

TouchInputManager::TouchInputManager(TouchMapping mapping) : mapping_(std::move(mapping)) {}

const TouchMapping& TouchInputManager::mapping() const { return mapping_; }

std::string TouchInputManager::eventNode() const {
    const std::filesystem::path root("/sys/class/input");
    std::error_code ec;
    if (!std::filesystem::is_directory(root, ec)) {
        return {};
    }
    for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(root, ec)) {
        if (ec) {
            break;
        }
        const std::string name = entry.path().filename().string();
        if (name.rfind("event", 0) != 0) {
            continue;
        }
        if (readLine(entry.path() / "device" / "name") == mapping_.deviceName) {
            return "/dev/input/" + name;
        }
    }
    return {};
}

}  // namespace headunit
