#include "camera/StorageManager.hpp"

#include <QTest>

#include <cstdint>
#include <string>
#include <vector>

using headunit::ClipSegment;
using headunit::EvictionPlan;
using headunit::StorageManager;
using headunit::StoragePolicy;
using headunit::VolumeMount;

Q_DECLARE_METATYPE(std::vector<ClipSegment>)

namespace {

EvictionPlan evict(const std::vector<ClipSegment>& clips, std::uint64_t limit, std::uint64_t reserve,
                   std::uint64_t next) {
    StorageManager storage;
    return storage.planEviction(clips, StoragePolicy{limit, reserve}, next);
}

}  // namespace

class StorageTest : public QObject {
    Q_OBJECT

private slots:
    void refusals_data() {
        QTest::addColumn<std::vector<ClipSegment>>("clips");
        QTest::addColumn<qulonglong>("limit");
        QTest::addColumn<qulonglong>("reserve");
        QTest::addColumn<qulonglong>("next");
        QTest::addColumn<QString>("reason");
        QTest::newRow("no disk") << std::vector<ClipSegment>{} << 0ull << 0ull << 100ull
                                 << "no footage volume configured";
        QTest::newRow("reserve") << std::vector<ClipSegment>{} << 100ull << 100ull << 10ull
                                 << "reserve leaves no usable space";
        QTest::newRow("zero clip") << std::vector<ClipSegment>{} << 1000ull << 0ull << 0ull << "next clip size is zero";
        QTest::newRow("too big") << std::vector<ClipSegment>{{"old", 50}} << 100ull << 0ull << 150ull
                                 << "segment larger than usable budget";
        QTest::newRow("empty name") << std::vector<ClipSegment>{{"", 10}} << 100ull << 0ull << 10ull
                                    << "clip name is empty";
        QTest::newRow("overflow") << std::vector<ClipSegment>{{"a", UINT64_MAX}, {"b", UINT64_MAX}}
                                  << qulonglong(UINT64_MAX) << 0ull << 1ull << "clip sizes overflow";
    }

    void refusals() {
        QFETCH(std::vector<ClipSegment>, clips);
        QFETCH(qulonglong, limit);
        QFETCH(qulonglong, reserve);
        QFETCH(qulonglong, next);
        QFETCH(QString, reason);
        const auto plan = evict(clips, limit, reserve, next);
        QVERIFY(!plan.accepted);
        QCOMPARE(QString::fromStdString(plan.reason), reason);
        QVERIFY(plan.removeOldestFirst.empty());
    }

    void exactFit() {
        const auto plan = evict({{"a", 100}, {"b", 100}}, 300, 0, 100);
        QVERIFY(plan.accepted);
        QVERIFY(plan.removeOldestFirst.empty());
        QCOMPARE(plan.bytesUsedAfter, 300ull);
    }

    void dropsOldestFirst() {
        const auto plan = evict({{"old", 100}, {"mid", 100}, {"new", 100}}, 250, 0, 100);
        QVERIFY(plan.accepted);
        QCOMPARE(plan.removeOldestFirst, (std::vector<std::string>{"old", "mid"}));
        QCOMPARE(plan.bytesUsedAfter, 200ull);
    }

    void reserveCountsAgainstLimit() {
        const auto plan = evict({{"old", 500}, {"new", 500}}, 1000, 400, 200);
        QVERIFY(plan.accepted);
        QCOMPARE(plan.removeOldestFirst.size(), std::size_t(2));
        QCOMPARE(plan.bytesUsedAfter, 200ull);
    }

    void neverExceedsBudget() {
        // Property: an accepted plan always fits in limit - reserve.
        std::vector<ClipSegment> clips;
        for (int i = 0; i < 200; ++i) {
            // Appending avoids GCC 12's false -Wrestrict on "literal" + std::string.
            std::string name = std::to_string(i);
            name.insert(name.begin(), 'c');
            clips.push_back({name, static_cast<std::uint64_t>(37 * (i % 11) + 1)});
        }
        for (std::uint64_t limit = 0; limit < 3000; limit += 97) {
            for (std::uint64_t reserve : {0ull, 50ull, 500ull}) {
                const auto plan = evict(clips, limit, reserve, 120);
                if (plan.accepted) {
                    QVERIFY(plan.bytesUsedAfter <= limit - reserve);
                }
            }
        }
    }

    void footageVolume_data() {
        QTest::addColumn<QString>("dir");
        QTest::addColumn<QString>("osSource");
        QTest::addColumn<bool>("allowed");
        QTest::addColumn<QString>("reason");
        QTest::newRow("empty") << "" << "/dev/mmcblk0p2" << false << "clips directory is empty";
        QTest::newRow("home") << "/home/kagamirudo/clips" << "/dev/mmcblk0p2" << false
                              << "clips directory is on the OS disk";
        QTest::newRow("usb sub") << "/mnt/footage/clips" << "/dev/mmcblk0p2" << true << "";
        QTest::newRow("usb root") << "/mnt/footage" << "/dev/mmcblk0p2" << true << "";
        QTest::newRow("prefix trick") << "/mnt/foobar/clips" << "/dev/mmcblk0p2" << false
                                      << "clips directory is on the OS disk";
        QTest::newRow("relative") << "relative/clips" << "/dev/mmcblk0p2" << false
                                  << "clips directory must be absolute";
        QTest::newRow("unknown os") << "/mnt/footage/clips" << "" << false << "OS volume is unknown";
    }

    void footageVolume() {
        QFETCH(QString, dir);
        QFETCH(QString, osSource);
        QFETCH(bool, allowed);
        QFETCH(QString, reason);
        StorageManager storage;
        const std::vector<VolumeMount> mounts = {
            {"/", "/dev/mmcblk0p2"},
            {"/mnt/footage", "/dev/sda1"},
            {"/mnt/foo", "/dev/sdb1"},
        };
        std::string why;
        QCOMPARE(storage.footageVolumeAllowed(dir.toStdString(), osSource.toStdString(), mounts, &why), allowed);
        QCOMPARE(QString::fromStdString(why), reason);
    }
};

QTEST_GUILESS_MAIN(StorageTest)
#include "tst_storage.moc"
