#include "camera/StorageManager.hpp"

namespace headunit {
namespace {

bool add(std::uint64_t a, std::uint64_t b, std::uint64_t* out) {
    if (a > UINT64_MAX - b) {
        return false;
    }
    *out = a + b;
    return true;
}

std::string trimTrailingSlash(std::string path) {
    while (path.size() > 1 && path.back() == '/') {
        path.pop_back();
    }
    return path;
}

bool pathOnMount(const std::string& path, const std::string& mountPoint) {
    const std::string mount = trimTrailingSlash(mountPoint);
    const std::string clean = trimTrailingSlash(path);
    if (mount.empty() || mount.front() != '/') {
        return false;
    }
    if (mount == "/") {
        return clean.empty() || clean.front() == '/';
    }
    if (clean == mount) {
        return true;
    }
    return clean.size() > mount.size() && clean.compare(0, mount.size(), mount) == 0 && clean[mount.size()] == '/';
}

}  // namespace

EvictionPlan StorageManager::planEviction(const std::vector<ClipSegment>& oldestFirst, StoragePolicy policy,
                                          std::uint64_t nextClipBytes) const {
    EvictionPlan plan;
    if (policy.limitBytes == 0) {
        plan.reason = "no footage volume configured";
        return plan;
    }
    if (policy.reserveBytes >= policy.limitBytes) {
        plan.reason = "reserve leaves no usable space";
        return plan;
    }
    const std::uint64_t usable = policy.limitBytes - policy.reserveBytes;
    if (nextClipBytes == 0) {
        plan.reason = "next clip size is zero";
        return plan;
    }
    if (nextClipBytes > usable) {
        plan.reason = "segment larger than usable budget";
        return plan;
    }

    std::uint64_t used = 0;
    for (const ClipSegment& clip : oldestFirst) {
        if (clip.name.empty()) {
            plan.reason = "clip name is empty";
            return plan;
        }
        if (!add(used, clip.bytes, &used)) {
            plan.reason = "clip sizes overflow";
            return plan;
        }
    }

    std::uint64_t droppedBytes = 0;
    for (const ClipSegment& clip : oldestFirst) {
        std::uint64_t kept = used - droppedBytes;
        std::uint64_t need = 0;
        if (!add(kept, nextClipBytes, &need)) {
            plan.reason = "clip sizes overflow";
            return plan;
        }
        if (need <= usable) {
            break;
        }
        if (!add(droppedBytes, clip.bytes, &droppedBytes)) {
            plan.reason = "clip sizes overflow";
            return plan;
        }
        plan.removeOldestFirst.push_back(clip.name);
    }

    const std::uint64_t kept = used - droppedBytes;
    std::uint64_t after = 0;
    if (!add(kept, nextClipBytes, &after) || after > usable) {
        plan.removeOldestFirst.clear();
        plan.bytesUsedAfter = 0;
        plan.accepted = false;
        plan.reason = "segment does not fit after eviction";
        return plan;
    }
    plan.accepted = true;
    plan.bytesUsedAfter = after;
    plan.reason = "ok";
    return plan;
}

bool StorageManager::footageVolumeAllowed(const std::string& clipsDirectory, const std::string& osSource,
                                          const std::vector<VolumeMount>& mounts, std::string* reason) const {
    auto set = [&](const char* text) {
        if (reason) {
            *reason = text;
        }
    };
    if (clipsDirectory.empty()) {
        set("clips directory is empty");
        return false;
    }
    if (osSource.empty()) {
        set("OS volume is unknown");
        return false;
    }
    if (clipsDirectory.front() != '/') {
        set("clips directory must be absolute");
        return false;
    }

    const VolumeMount* best = nullptr;
    std::size_t bestLen = 0;
    for (const VolumeMount& mount : mounts) {
        if (!pathOnMount(clipsDirectory, mount.mountPoint)) {
            continue;
        }
        const std::size_t len = trimTrailingSlash(mount.mountPoint).size();
        if (!best || len > bestLen) {
            best = &mount;
            bestLen = len;
        }
    }
    if (!best) {
        set("clips directory is not on a known mount");
        return false;
    }
    if (best->source.empty()) {
        set("footage mount has no source");
        return false;
    }
    if (best->source == osSource) {
        set("clips directory is on the OS disk");
        return false;
    }
    if (reason) {
        reason->clear();
    }
    return true;
}

}  // namespace headunit
