#pragma once

#include <string_view>

namespace headunit {

class VideoRenderer {
public:
    bool hasFrame() const;
    std::string_view backend() const;
    std::string_view detail() const;
};

}  // namespace headunit
