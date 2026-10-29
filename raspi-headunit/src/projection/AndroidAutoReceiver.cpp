#include "projection/AndroidAutoReceiver.hpp"

namespace headunit {

std::string_view AndroidAutoReceiver::id() const { return "android-auto"; }

std::string_view AndroidAutoReceiver::displayName() const { return "Android Auto"; }

FeatureMaturity AndroidAutoReceiver::maturity() const { return FeatureMaturity::Planned; }

SessionState AndroidAutoReceiver::session() const { return session_; }

std::string_view AndroidAutoReceiver::detail() const { return detail_; }

bool AndroidAutoReceiver::start() {
    session_ = SessionState::Inactive;
    detail_ = "Android Auto receiver is not implemented.";
    return false;
}

void AndroidAutoReceiver::stop() {
    session_ = SessionState::Inactive;
    detail_ = "Android Auto receiver is not implemented.";
}

}  // namespace headunit
