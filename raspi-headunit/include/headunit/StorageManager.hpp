#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace headunit {

struct RetentionPlan {
  std::uint64_t usableBytes = 0;
  std::uint64_t maxSegments = 0;
  // Oldest names that must be removed so the clip count fits maxSegments.
  std::vector<std::string> dropOldestFirst;
};

// capacity and reserve are bytes. segmentBytes is one clip file.
// oldestFirst is clip names, oldest at the front.
RetentionPlan planRetention(std::uint64_t capacityBytes,
                            std::uint64_t reserveBytes,
                            std::uint64_t segmentBytes,
                            const std::vector<std::string>& oldestFirst);

}  // namespace headunit
