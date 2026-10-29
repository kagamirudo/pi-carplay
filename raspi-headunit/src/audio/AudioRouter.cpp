#include "audio/AudioRouter.hpp"

namespace headunit {

const char* audioRouteName(AudioRoute route) {
    switch (route) {
        case AudioRoute::None:
            return "none";
        case AudioRoute::LocalUi:
            return "local";
        case AudioRoute::Projection:
            return "projection";
    }
    return "none";
}

AudioRoute AudioRouter::route() const { return route_; }

std::string_view AudioRouter::sink() const { return "not configured"; }

std::string_view AudioRouter::detail() const {
    return "PipeWire routing to the car A2DP sink is not connected.";
}

void AudioRouter::setRoute(AudioRoute route) { route_ = route; }

}  // namespace headunit
