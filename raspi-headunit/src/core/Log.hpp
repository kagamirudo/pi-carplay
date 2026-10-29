#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace headunit {

enum class LogLevel { Debug, Info, Warn, Error };

const char* logLevelName(LogLevel level);
std::optional<LogLevel> parseLogLevel(std::string_view text);

// One JSON object per line on stderr, and on a file when a path is set.
class Logger {
public:
    static void setLevel(LogLevel level);
    static void setPath(const std::string& path);
    static void event(LogLevel level, std::string_view component, std::string_view message,
                      const std::vector<std::pair<std::string, std::string>>& fields = {});

    static void info(std::string_view component, std::string_view message);
    static void warn(std::string_view component, std::string_view message);
    static void error(std::string_view component, std::string_view message);
};

}  // namespace headunit
