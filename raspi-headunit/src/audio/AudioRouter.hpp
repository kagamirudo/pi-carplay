#pragma once

#include <string>
#include <string_view>

namespace headunit {

enum class AudioRoute { None, LocalUi, Projection };

class AudioRouter {
public:
    AudioRoute route() const;
    std::string_view sink() const;
    std::string_view detail() const;
    void setRoute(AudioRoute route);

private:
    AudioRoute route_ = AudioRoute::None;
};

const char* audioRouteName(AudioRoute route);

}  // namespace headunit
