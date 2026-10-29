#include "projection/CarPlayReceiver.hpp"

namespace headunit {

std::string_view CarPlayReceiver::id() const { return "carplay"; }

std::string_view CarPlayReceiver::displayName() const { return "CarPlay"; }

FeatureMaturity CarPlayReceiver::maturity() const { return FeatureMaturity::Planned; }

SessionState CarPlayReceiver::session() const { return session_; }

std::string_view CarPlayReceiver::detail() const { return detail_; }

bool CarPlayReceiver::start() {
    session_ = SessionState::Inactive;
    detail_ = "CarPlay receiver is not implemented.";
    return false;
}

void CarPlayReceiver::stop() {
    session_ = SessionState::Inactive;
    detail_ = "CarPlay receiver is not implemented.";
}

}  // namespace headunit
