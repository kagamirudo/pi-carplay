#include "core/Log.hpp"

#include <chrono>
#include <cstdio>
#include <ctime>
#include <fstream>
#include <iostream>
#include <mutex>

namespace headunit {
namespace {

std::mutex g_mu;
LogLevel g_level = LogLevel::Info;
std::ofstream g_file;

int rank(LogLevel level) {
    switch (level) {
        case LogLevel::Debug:
            return 0;
        case LogLevel::Info:
            return 1;
        case LogLevel::Warn:
            return 2;
        case LogLevel::Error:
            return 3;
    }
    return 1;
}

void appendEscaped(std::string* out, std::string_view text) {
    for (unsigned char c : text) {
        switch (c) {
            case '"':
                *out += "\\\"";
                break;
            case '\\':
                *out += "\\\\";
                break;
            case '\n':
                *out += "\\n";
                break;
            case '\r':
                *out += "\\r";
                break;
            case '\t':
                *out += "\\t";
                break;
            default:
                if (c < 0x20) {
                    constexpr char hex[] = "0123456789abcdef";
                    *out += "\\u00";
                    *out += hex[c >> 4];
                    *out += hex[c & 0xf];
                } else {
                    out->push_back(static_cast<char>(c));
                }
                break;
        }
    }
}

std::string timestampUtc() {
    using clock = std::chrono::system_clock;
    const auto now = clock::now();
    const auto secs = std::chrono::time_point_cast<std::chrono::seconds>(now);
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - secs).count();
    const std::time_t tt = clock::to_time_t(now);
    std::tm tm{};
    gmtime_r(&tt, &tm);
    char buf[32];
    const int n = std::snprintf(buf, sizeof(buf), "%04d-%02d-%02dT%02d:%02d:%02d.%03dZ",
                                 tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min,
                                 tm.tm_sec, static_cast<int>(ms));
    return std::string(buf, buf + (n > 0 ? n : 0));
}

}  // namespace

const char* logLevelName(LogLevel level) {
    switch (level) {
        case LogLevel::Debug:
            return "debug";
        case LogLevel::Info:
            return "info";
        case LogLevel::Warn:
            return "warn";
        case LogLevel::Error:
            return "error";
    }
    return "info";
}

std::optional<LogLevel> parseLogLevel(std::string_view text) {
    if (text == "debug") {
        return LogLevel::Debug;
    }
    if (text == "info") {
        return LogLevel::Info;
    }
    if (text == "warn" || text == "warning") {
        return LogLevel::Warn;
    }
    if (text == "error") {
        return LogLevel::Error;
    }
    return std::nullopt;
}

void Logger::setLevel(LogLevel level) {
    std::lock_guard lock(g_mu);
    g_level = level;
}

void Logger::setPath(const std::string& path) {
    std::lock_guard lock(g_mu);
    if (g_file.is_open()) {
        g_file.close();
    }
    if (path.empty()) {
        return;
    }
    g_file.open(path, std::ios::app);
    if (!g_file) {
        std::cerr << "{\"level\":\"error\",\"component\":\"log\",\"msg\":\"could not open log file\"}\n";
    }
}

void Logger::event(LogLevel level, std::string_view component, std::string_view message,
                   const std::vector<std::pair<std::string, std::string>>& fields) {
    std::lock_guard lock(g_mu);
    if (rank(level) < rank(g_level)) {
        return;
    }
    std::string line;
    line.reserve(160 + message.size());
    line += "{\"ts\":\"";
    line += timestampUtc();
    line += "\",\"level\":\"";
    line += logLevelName(level);
    line += "\",\"component\":\"";
    appendEscaped(&line, component);
    line += "\",\"msg\":\"";
    appendEscaped(&line, message);
    line += '"';
    if (!fields.empty()) {
        line += ",\"fields\":{";
        for (std::size_t i = 0; i < fields.size(); ++i) {
            if (i != 0) {
                line += ',';
            }
            line += '"';
            appendEscaped(&line, fields[i].first);
            line += "\":\"";
            appendEscaped(&line, fields[i].second);
            line += '"';
        }
        line += '}';
    }
    line += "}\n";
    std::cerr << line;
    if (g_file.is_open()) {
        g_file << line;
        g_file.flush();
    }
}

void Logger::info(std::string_view component, std::string_view message) {
    event(LogLevel::Info, component, message);
}

void Logger::warn(std::string_view component, std::string_view message) {
    event(LogLevel::Warn, component, message);
}

void Logger::error(std::string_view component, std::string_view message) {
    event(LogLevel::Error, component, message);
}

}  // namespace headunit
