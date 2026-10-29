#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace headunit {

struct ClipSegment {
    std::string name;
    std::uint64_t bytes = 0;
};

struct StoragePolicy {
    std::uint64_t limitBytes = 0;
    std::uint64_t reserveBytes = 0;
};

struct EvictionPlan {
    std::vector<std::string> removeOldestFirst;
    std::uint64_t bytesUsedAfter = 0;
    bool accepted = false;
    std::string reason;
};

struct VolumeMount {
    std::string mountPoint;
    std::string source;
};

// Pure storage policy. This does not open a camera or delete files.
class StorageManager {
public:
    EvictionPlan planEviction(const std::vector<ClipSegment>& oldestFirst, StoragePolicy policy,
                              std::uint64_t nextClipBytes) const;

    // True only when clipsDirectory sits on a mount whose source is not the OS disk.
    bool footageVolumeAllowed(const std::string& clipsDirectory, const std::string& osSource,
                              const std::vector<VolumeMount>& mounts, std::string* reason) const;
};

}  // namespace headunit
