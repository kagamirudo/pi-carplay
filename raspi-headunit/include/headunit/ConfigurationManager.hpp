#pragma once

#include <string>
#include <string_view>
#include <unordered_map>

namespace headunit {

// Reads key=value lines. Blank lines and lines starting with # are ignored.
// A missing file leaves the map empty so callers can use their own defaults.
class ConfigurationManager {
 public:
  explicit ConfigurationManager(std::string path = {});

  bool load();
  bool has(std::string_view key) const;
  std::string getString(std::string_view key, std::string_view fallback) const;
  int getInt(std::string_view key, int fallback) const;

  const std::string& path() const { return path_; }

 private:
  std::string path_;
  std::unordered_map<std::string, std::string> values_;
};

}  // namespace headunit
