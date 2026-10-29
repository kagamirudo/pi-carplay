#include "headunit/ConfigurationManager.hpp"
#include "headunit/StorageManager.hpp"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>

namespace {

int failures = 0;

void expect(bool condition, const char* message) {
  if (!condition) {
    std::cerr << "FAIL " << message << '\n';
    ++failures;
  }
}

void testConfig() {
  const std::string path = "test_headunit.conf";
  {
    std::ofstream out(path);
    out << "# comment\n\n";
    out << " display_width = 480 \n";
    out << "rotation=180\n";
    out << "not a pair\n";
    out << "badint=nope\n";
  }
  headunit::ConfigurationManager config(path);
  expect(config.load(), "load existing file");
  expect(config.getInt("display_width", 0) == 480, "trimmed int");
  expect(config.getInt("rotation", 0) == 180, "rotation");
  expect(config.getInt("missing", 7) == 7, "default int");
  expect(config.getInt("badint", 3) == 3, "bad int falls back");
  expect(config.getString("nope", "usb") == "usb", "default string");
  expect(!config.has("not"), "malformed line ignored");

  headunit::ConfigurationManager missing("does-not-exist.conf");
  expect(!missing.load(), "missing file");
  expect(missing.getInt("display_width", 480) == 480, "defaults without file");
  std::remove(path.c_str());
}

void testRetention() {
  const std::vector<std::string> clips = {"a", "b", "c", "d", "e"};
  const auto fit = headunit::planRetention(1000, 100, 100, clips);
  expect(fit.usableBytes == 900, "usable bytes");
  expect(fit.maxSegments == 9, "nine segments");
  expect(fit.dropOldestFirst.empty(), "nothing to drop");

  const auto tight = headunit::planRetention(1000, 100, 300, clips);
  expect(tight.maxSegments == 3, "three segments");
  expect(tight.dropOldestFirst.size() == 2, "drop two oldest");
  expect(tight.dropOldestFirst[0] == "a" && tight.dropOldestFirst[1] == "b",
         "oldest first");

  const auto emptyDisk = headunit::planRetention(50, 100, 10, clips);
  expect(emptyDisk.usableBytes == 0, "reserve larger than disk");
  expect(emptyDisk.maxSegments == 0, "no room");
  expect(emptyDisk.dropOldestFirst.size() == clips.size(), "drop every clip");

  const auto zeroSeg = headunit::planRetention(1000, 0, 0, clips);
  expect(zeroSeg.maxSegments == 0, "zero-length segment stores nothing");
}

}  // namespace

int main() {
  testConfig();
  testRetention();
  if (failures != 0) {
    std::cerr << failures << " failed\n";
    return 1;
  }
  std::cout << "ok\n";
  return 0;
}
