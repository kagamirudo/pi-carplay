#include "camera/StorageManager.hpp"
#include "test_harness.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace {

headunit::EvictionPlan evict(const std::vector<headunit::ClipSegment>& clips, std::uint64_t limit,
                             std::uint64_t reserve, std::uint64_t next) {
    headunit::StorageManager storage;
    return storage.planEviction(clips, headunit::StoragePolicy{limit, reserve}, next);
}

void testEviction() {
    const auto noDisk = evict({}, 0, 0, 100);
    EXPECT(!noDisk.accepted);
    EXPECT(noDisk.reason == "no footage volume configured");
    EXPECT(noDisk.removeOldestFirst.empty());

    const auto reserve = evict({}, 100, 100, 10);
    EXPECT(!reserve.accepted);
    EXPECT(reserve.reason == "reserve leaves no usable space");

    const auto zero = evict({}, 1000, 0, 0);
    EXPECT(!zero.accepted);
    EXPECT(zero.reason == "next clip size is zero");

    const auto tooBig = evict({{"old", 50}}, 100, 0, 150);
    EXPECT(!tooBig.accepted);
    EXPECT(tooBig.reason == "segment larger than usable budget");
    EXPECT(tooBig.removeOldestFirst.empty());

    const auto exact = evict({{"a", 100}, {"b", 100}}, 300, 0, 100);
    EXPECT(exact.accepted);
    EXPECT(exact.removeOldestFirst.empty());
    EXPECT(exact.bytesUsedAfter == 300);

    const auto dropTwo = evict({{"old", 100}, {"mid", 100}, {"new", 100}}, 250, 0, 100);
    EXPECT(dropTwo.accepted);
    EXPECT(dropTwo.removeOldestFirst.size() == 2);
    EXPECT(dropTwo.removeOldestFirst[0] == "old");
    EXPECT(dropTwo.removeOldestFirst[1] == "mid");
    EXPECT(dropTwo.bytesUsedAfter == 200);

    const auto withReserve = evict({{"old", 500}, {"new", 500}}, 1000, 400, 200);
    EXPECT(withReserve.accepted);
    EXPECT(withReserve.removeOldestFirst.size() == 2);
    EXPECT(withReserve.bytesUsedAfter == 200);

    const auto emptyName = evict({{"", 10}}, 100, 0, 10);
    EXPECT(!emptyName.accepted);
    EXPECT(emptyName.reason == "clip name is empty");

    const auto overflow = evict({{"a", UINT64_MAX}, {"b", UINT64_MAX}}, UINT64_MAX, 0, 1);
    EXPECT(!overflow.accepted);
    EXPECT(overflow.reason == "clip sizes overflow");
}

void testVolume() {
    headunit::StorageManager storage;
    const std::vector<headunit::VolumeMount> mounts = {
        {"/", "/dev/mmcblk0p2"},
        {"/mnt/footage", "/dev/sda1"},
        {"/mnt/foo", "/dev/sdb1"},
    };
    std::string reason;

    EXPECT(!storage.footageVolumeAllowed("", "/dev/mmcblk0p2", mounts, &reason));
    EXPECT(reason == "clips directory is empty");

    EXPECT(!storage.footageVolumeAllowed("/home/kagamirudo/clips", "/dev/mmcblk0p2", mounts, &reason));
    EXPECT(reason == "clips directory is on the OS disk");

    EXPECT(storage.footageVolumeAllowed("/mnt/footage/clips", "/dev/mmcblk0p2", mounts, &reason));
    EXPECT(reason.empty());

    EXPECT(storage.footageVolumeAllowed("/mnt/footage", "/dev/mmcblk0p2", mounts, &reason));

    EXPECT(!storage.footageVolumeAllowed("/mnt/foobar/clips", "/dev/mmcblk0p2", mounts, &reason));
    EXPECT(reason == "clips directory is on the OS disk");

    EXPECT(!storage.footageVolumeAllowed("relative/clips", "/dev/mmcblk0p2", mounts, &reason));
    EXPECT(reason == "clips directory must be absolute");

    EXPECT(!storage.footageVolumeAllowed("/mnt/footage/clips", "", mounts, &reason));
    EXPECT(reason == "OS volume is unknown");
}

}  // namespace

void testStorage() {
    testEviction();
    testVolume();
}
