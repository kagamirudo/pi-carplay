#pragma once

#include "projection/AndroidAutoReceiver.hpp"
#include "projection/CarPlayReceiver.hpp"

#include <string_view>

namespace headunit {

class ProjectionManager {
public:
    ProjectionReceiver& androidAuto();
    ProjectionReceiver& carPlay();
    const ProjectionReceiver& androidAuto() const;
    const ProjectionReceiver& carPlay() const;
    ProjectionReceiver* active();
    std::string_view screenOwner() const;

private:
    AndroidAutoReceiver androidAuto_;
    CarPlayReceiver carPlay_;
};

}  // namespace headunit
