#pragma once

#include "projection/ProjectionReceiver.hpp"

#include <string>

namespace headunit {

// Stub. No CarPlay protocol and no MFi authentication.
class CarPlayReceiver final : public ProjectionReceiver {
public:
    std::string_view id() const override;
    std::string_view displayName() const override;
    FeatureMaturity maturity() const override;
    SessionState session() const override;
    std::string_view detail() const override;
    bool start() override;
    void stop() override;

private:
    SessionState session_ = SessionState::Inactive;
    std::string detail_ = "CarPlay receiver is not implemented.";
};

}  // namespace headunit
