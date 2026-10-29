#pragma once

#include <string_view>

namespace headunit {

enum class LogLevel { Info, Warn, Error };

void log(LogLevel level, std::string_view message);

}  // namespace headunit
