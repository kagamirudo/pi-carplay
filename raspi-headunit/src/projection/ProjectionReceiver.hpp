#pragma once

#include <string>
#include <string_view>

namespace headunit {

enum class FeatureMaturity { Planned, Experimental, Working };
enum class SessionState { Inactive, Starting, Active, Failed };

class ProjectionReceiver {
public:
    virtual ~ProjectionReceiver() = default;
    virtual std::string_view id() const = 0;
    virtual std::string_view displayName() const = 0;
    virtual FeatureMaturity maturity() const = 0;
    virtual SessionState session() const = 0;
    virtual std::string_view detail() const = 0;
    virtual bool start() = 0;
    virtual void stop() = 0;
};

const char* maturityName(FeatureMaturity maturity);
const char* sessionName(SessionState state);

}  // namespace headunit
