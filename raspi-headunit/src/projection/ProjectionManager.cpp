#include "projection/ProjectionManager.hpp"

namespace headunit {

const char* maturityName(FeatureMaturity maturity) {
    switch (maturity) {
        case FeatureMaturity::Planned:
            return "planned";
        case FeatureMaturity::Experimental:
            return "experimental";
        case FeatureMaturity::Working:
            return "working";
    }
    return "planned";
}

const char* sessionName(SessionState state) {
    switch (state) {
        case SessionState::Inactive:
            return "inactive";
        case SessionState::Starting:
            return "starting";
        case SessionState::Active:
            return "active";
        case SessionState::Failed:
            return "failed";
    }
    return "inactive";
}

ProjectionReceiver& ProjectionManager::androidAuto() { return androidAuto_; }

ProjectionReceiver& ProjectionManager::carPlay() { return carPlay_; }

const ProjectionReceiver& ProjectionManager::androidAuto() const { return androidAuto_; }

const ProjectionReceiver& ProjectionManager::carPlay() const { return carPlay_; }

ProjectionReceiver* ProjectionManager::active() {
    if (androidAuto_.session() == SessionState::Active) {
        return &androidAuto_;
    }
    if (carPlay_.session() == SessionState::Active) {
        return &carPlay_;
    }
    return nullptr;
}

std::string_view ProjectionManager::screenOwner() const { return "shell"; }

}  // namespace headunit
