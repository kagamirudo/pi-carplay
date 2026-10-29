#include "core/ConfigurationManager.hpp"

#include "core/Log.hpp"

#include <fcntl.h>
#include <unistd.h>

#include <cctype>
#include <fstream>
#include <map>
#include <sstream>
#include <utility>

namespace headunit {
namespace {

struct Json {
    enum class Type { Null, Bool, Number, String, Object };
    Type type = Type::Null;
    bool boolean = false;
    std::uint64_t number = 0;
    bool negative = false;
    std::string text;
    std::map<std::string, Json> object;
};

class Parser {
public:
    explicit Parser(std::string_view in) : in_(in) {}

    bool parse(Json* out) {
        skip();
        if (failed_ || i_ >= in_.size()) {
            return fail("empty config");
        }
        if (!parseValue(out)) {
            return false;
        }
        skip();
        if (i_ != in_.size()) {
            return fail("trailing data after JSON");
        }
        return !failed_;
    }

    const std::string& error() const { return error_; }

private:
    bool fail(const std::string& message) {
        if (!failed_) {
            failed_ = true;
            error_ = message;
        }
        return false;
    }

    void skip() {
        while (i_ < in_.size() && std::isspace(static_cast<unsigned char>(in_[i_]))) {
            ++i_;
        }
    }

    bool consume(char expected) {
        if (i_ < in_.size() && in_[i_] == expected) {
            ++i_;
            return true;
        }
        return fail(std::string("expected '") + expected + "'");
    }

    bool parseValue(Json* out) {
        if (failed_ || i_ >= in_.size()) {
            return fail("unexpected end of JSON");
        }
        const char c = in_[i_];
        if (c == '{') {
            return parseObject(out);
        }
        if (c == '"') {
            out->type = Json::Type::String;
            return parseString(&out->text);
        }
        if (c == 't') {
            if (!literal("true")) {
                return false;
            }
            out->type = Json::Type::Bool;
            out->boolean = true;
            return true;
        }
        if (c == 'f') {
            if (!literal("false")) {
                return false;
            }
            out->type = Json::Type::Bool;
            out->boolean = false;
            return true;
        }
        if (c == 'n') {
            if (!literal("null")) {
                return false;
            }
            out->type = Json::Type::Null;
            return true;
        }
        if (c == '-' || (c >= '0' && c <= '9')) {
            return parseNumber(out);
        }
        return fail("unexpected JSON value");
    }

    bool literal(const char* word) {
        for (const char* p = word; *p; ++p) {
            if (i_ >= in_.size() || in_[i_] != *p) {
                return fail(std::string("expected ") + word);
            }
            ++i_;
        }
        return true;
    }

    bool parseNumber(Json* out) {
        out->type = Json::Type::Number;
        if (in_[i_] == '-') {
            out->negative = true;
            ++i_;
            if (i_ >= in_.size() || in_[i_] < '0' || in_[i_] > '9') {
                return fail("expected digit");
            }
        }
        if (in_[i_] == '0') {
            ++i_;
            if (i_ < in_.size() && in_[i_] >= '0' && in_[i_] <= '9') {
                return fail("leading zero");
            }
        } else {
            std::uint64_t value = 0;
            if (in_[i_] < '1' || in_[i_] > '9') {
                return fail("expected digit");
            }
            while (i_ < in_.size() && in_[i_] >= '0' && in_[i_] <= '9') {
                const int digit = in_[i_] - '0';
                if (value > (UINT64_MAX - static_cast<unsigned>(digit)) / 10) {
                    return fail("number overflow");
                }
                value = value * 10 + static_cast<unsigned>(digit);
                ++i_;
            }
            out->number = value;
        }
        if (i_ < in_.size() && (in_[i_] == '.' || in_[i_] == 'e' || in_[i_] == 'E')) {
            return fail("fractional numbers are not used");
        }
        return true;
    }

    bool parseString(std::string* out) {
        if (!consume('"')) {
            return false;
        }
        while (i_ < in_.size()) {
            const unsigned char c = static_cast<unsigned char>(in_[i_]);
            if (c == '"') {
                ++i_;
                return true;
            }
            if (c == '\\') {
                ++i_;
                if (i_ >= in_.size()) {
                    return fail("bad string escape");
                }
                const char e = in_[i_++];
                switch (e) {
                    case '"':
                    case '\\':
                    case '/':
                        out->push_back(e);
                        break;
                    case 'b':
                        out->push_back('\b');
                        break;
                    case 'f':
                        out->push_back('\f');
                        break;
                    case 'n':
                        out->push_back('\n');
                        break;
                    case 'r':
                        out->push_back('\r');
                        break;
                    case 't':
                        out->push_back('\t');
                        break;
                    case 'u':
                        if (!parseUnicode(out)) {
                            return false;
                        }
                        break;
                    default:
                        return fail("bad string escape");
                }
                continue;
            }
            if (c < 0x20) {
                return fail("unescaped control character");
            }
            out->push_back(static_cast<char>(c));
            ++i_;
        }
        return fail("unterminated string");
    }

    bool parseUnicode(std::string* out) {
        if (i_ + 4 > in_.size()) {
            return fail("short unicode escape");
        }
        int code = 0;
        for (int n = 0; n < 4; ++n) {
            const char h = in_[i_++];
            code <<= 4;
            if (h >= '0' && h <= '9') {
                code += h - '0';
            } else if (h >= 'a' && h <= 'f') {
                code += h - 'a' + 10;
            } else if (h >= 'A' && h <= 'F') {
                code += h - 'A' + 10;
            } else {
                return fail("bad unicode escape");
            }
        }
        if (code < 0x80) {
            out->push_back(static_cast<char>(code));
        } else if (code < 0x800) {
            out->push_back(static_cast<char>(0xc0 | (code >> 6)));
            out->push_back(static_cast<char>(0x80 | (code & 0x3f)));
        } else {
            out->push_back(static_cast<char>(0xe0 | (code >> 12)));
            out->push_back(static_cast<char>(0x80 | ((code >> 6) & 0x3f)));
            out->push_back(static_cast<char>(0x80 | (code & 0x3f)));
        }
        return true;
    }

    bool parseObject(Json* out) {
        out->type = Json::Type::Object;
        if (!consume('{')) {
            return false;
        }
        if (++depth_ > kMaxDepth) {
            return fail("JSON nested too deeply");
        }
        const bool ok = parseMembers(out);
        --depth_;
        return ok;
    }

    bool parseMembers(Json* out) {
        skip();
        if (i_ < in_.size() && in_[i_] == '}') {
            ++i_;
            return true;
        }
        while (!failed_) {
            skip();
            std::string key;
            if (!parseString(&key)) {
                return false;
            }
            if (out->object.find(key) != out->object.end()) {
                return fail("duplicate key \"" + key + "\"");
            }
            skip();
            if (!consume(':')) {
                return false;
            }
            skip();
            Json value;
            if (!parseValue(&value)) {
                return false;
            }
            out->object.emplace(std::move(key), std::move(value));
            skip();
            if (i_ < in_.size() && in_[i_] == ',') {
                ++i_;
                continue;
            }
            if (i_ < in_.size() && in_[i_] == '}') {
                ++i_;
                return true;
            }
            return fail("expected ',' or '}'");
        }
        return false;
    }

    // Recursion bound so hostile or corrupt input cannot exhaust the stack.
    static constexpr int kMaxDepth = 16;

    std::string_view in_;
    std::size_t i_ = 0;
    int depth_ = 0;
    std::string error_;
    bool failed_ = false;
};

bool syncPath(const std::filesystem::path& path) {
    const int fd = ::open(path.c_str(), O_RDONLY | O_CLOEXEC);
    if (fd < 0) {
        return false;
    }
    const bool ok = ::fsync(fd) == 0;
    ::close(fd);
    return ok;
}

const Json* field(const Json& object, const char* key) {
    const auto it = object.object.find(key);
    if (it == object.object.end()) {
        return nullptr;
    }
    return &it->second;
}

bool requireObject(const Json& value, const char* path, std::string* error) {
    if (value.type == Json::Type::Object) {
        return true;
    }
    *error = std::string(path) + " must be an object";
    return false;
}

bool readBool(const Json& object, const char* key, const char* path, bool* dest, std::string* error) {
    const Json* value = field(object, key);
    if (!value) {
        return true;
    }
    if (value->type != Json::Type::Bool) {
        *error = std::string(path) + " must be a boolean";
        return false;
    }
    *dest = value->boolean;
    return true;
}

bool readString(const Json& object, const char* key, const char* path, std::string* dest, std::string* error) {
    const Json* value = field(object, key);
    if (!value) {
        return true;
    }
    if (value->type != Json::Type::String) {
        *error = std::string(path) + " must be a string";
        return false;
    }
    *dest = value->text;
    return true;
}

bool readUint(const Json& object, const char* key, const char* path, std::uint64_t* dest, bool allowZero,
              std::uint64_t maxValue, std::string* error) {
    const Json* value = field(object, key);
    if (!value) {
        return true;
    }
    if (value->type != Json::Type::Number) {
        *error = std::string(path) + " must be a number";
        return false;
    }
    if (value->negative || (value->number == 0 && !allowZero) || value->number > maxValue) {
        *error = std::string(path) + (allowZero ? " is out of range" : " must be positive");
        return false;
    }
    *dest = value->number;
    return true;
}

bool readInt(const Json& object, const char* key, const char* path, int* dest, int minValue, int maxValue,
             std::string* error) {
    std::uint64_t raw = 0;
    const Json* value = field(object, key);
    if (!value) {
        return true;
    }
    if (!readUint(object, key, path, &raw, minValue <= 0, static_cast<std::uint64_t>(maxValue), error)) {
        return false;
    }
    if (raw < static_cast<std::uint64_t>(minValue)) {
        *error = std::string(path) + " is out of range";
        return false;
    }
    *dest = static_cast<int>(raw);
    return true;
}

void escape(std::ostream& out, const std::string& text) {
    for (unsigned char c : text) {
        switch (c) {
            case '"':
                out << "\\\"";
                break;
            case '\\':
                out << "\\\\";
                break;
            case '\n':
                out << "\\n";
                break;
            case '\r':
                out << "\\r";
                break;
            case '\t':
                out << "\\t";
                break;
            default:
                if (c < 0x20) {
                    constexpr char hex[] = "0123456789abcdef";
                    out << "\\u00" << hex[c >> 4] << hex[c & 0xf];
                } else {
                    out << static_cast<char>(c);
                }
                break;
        }
    }
}

}  // namespace

AppConfig AppConfig::defaults() { return AppConfig{}; }

ParseResult ConfigurationManager::parse(std::string_view text) {
    ParseResult result;
    Parser parser(text);
    Json root;
    if (!parser.parse(&root)) {
        result.error = parser.error();
        return result;
    }
    if (root.type != Json::Type::Object) {
        result.error = "config must be a JSON object";
        return result;
    }

    AppConfig config = AppConfig::defaults();
    std::string error;
    if (const Json* display = field(root, "display")) {
        if (!requireObject(*display, "display", &error) ||
            !readInt(*display, "width", "display.width", &config.display.width, 1, 8192, &error) ||
            !readInt(*display, "height", "display.height", &config.display.height, 1, 8192, &error) ||
            !readString(*display, "output", "display.output", &config.display.output, &error) ||
            !readBool(*display, "fullscreen", "display.fullscreen", &config.display.fullscreen, &error)) {
            result.error = error;
            return result;
        }
        if (config.display.output.empty()) {
            result.error = "display.output must not be empty";
            return result;
        }
    }
    if (const Json* dashcam = field(root, "dashcam")) {
        if (!requireObject(*dashcam, "dashcam", &error) ||
            !readBool(*dashcam, "enabled", "dashcam.enabled", &config.dashcam.enabled, &error) ||
            !readInt(*dashcam, "width", "dashcam.width", &config.dashcam.width, 1, 8192, &error) ||
            !readInt(*dashcam, "height", "dashcam.height", &config.dashcam.height, 1, 8192, &error) ||
            !readInt(*dashcam, "fps", "dashcam.fps", &config.dashcam.fps, 1, 120, &error) ||
            !readInt(*dashcam, "segmentSeconds", "dashcam.segmentSeconds", &config.dashcam.segmentSeconds, 1, 3600,
                     &error) ||
            !readUint(*dashcam, "storageLimitBytes", "dashcam.storageLimitBytes", &config.dashcam.storageLimitBytes,
                      true, UINT64_MAX, &error) ||
            !readUint(*dashcam, "reserveBytes", "dashcam.reserveBytes", &config.dashcam.reserveBytes, true,
                      UINT64_MAX, &error) ||
            !readString(*dashcam, "clipsDirectory", "dashcam.clipsDirectory", &config.dashcam.clipsDirectory,
                        &error)) {
            result.error = error;
            return result;
        }
    }
    if (const Json* logging = field(root, "logging")) {
        if (!requireObject(*logging, "logging", &error) ||
            !readString(*logging, "level", "logging.level", &config.logging.level, &error) ||
            !readString(*logging, "path", "logging.path", &config.logging.path, &error)) {
            result.error = error;
            return result;
        }
        if (!parseLogLevel(config.logging.level)) {
            result.error = "logging.level must be debug, info, warn, or error";
            return result;
        }
        config.logging.level = logLevelName(*parseLogLevel(config.logging.level));
    }
    if (const Json* bluetooth = field(root, "bluetooth")) {
        if (!requireObject(*bluetooth, "bluetooth", &error) ||
            !readString(*bluetooth, "carDevice", "bluetooth.carDevice", &config.bluetooth.carDevice, &error)) {
            result.error = error;
            return result;
        }
    }

    result.ok = true;
    result.config = std::move(config);
    return result;
}

std::string ConfigurationManager::serialize(const AppConfig& config) {
    std::ostringstream out;
    out << "{\n";
    out << "  \"display\": {\n";
    out << "    \"width\": " << config.display.width << ",\n";
    out << "    \"height\": " << config.display.height << ",\n";
    out << "    \"output\": \"";
    escape(out, config.display.output);
    out << "\",\n";
    out << "    \"fullscreen\": " << (config.display.fullscreen ? "true" : "false") << "\n";
    out << "  },\n";
    out << "  \"dashcam\": {\n";
    out << "    \"enabled\": " << (config.dashcam.enabled ? "true" : "false") << ",\n";
    out << "    \"width\": " << config.dashcam.width << ",\n";
    out << "    \"height\": " << config.dashcam.height << ",\n";
    out << "    \"fps\": " << config.dashcam.fps << ",\n";
    out << "    \"segmentSeconds\": " << config.dashcam.segmentSeconds << ",\n";
    out << "    \"storageLimitBytes\": " << config.dashcam.storageLimitBytes << ",\n";
    out << "    \"reserveBytes\": " << config.dashcam.reserveBytes << ",\n";
    out << "    \"clipsDirectory\": \"";
    escape(out, config.dashcam.clipsDirectory);
    out << "\"\n";
    out << "  },\n";
    out << "  \"logging\": {\n";
    out << "    \"level\": \"";
    escape(out, config.logging.level);
    out << "\",\n";
    out << "    \"path\": \"";
    escape(out, config.logging.path);
    out << "\"\n";
    out << "  },\n";
    out << "  \"bluetooth\": {\n";
    out << "    \"carDevice\": \"";
    escape(out, config.bluetooth.carDevice);
    out << "\"\n";
    out << "  }\n";
    out << "}\n";
    return out.str();
}

void ConfigurationManager::reset(AppConfig config) { config_ = std::move(config); }

const AppConfig& ConfigurationManager::get() const { return config_; }

bool ConfigurationManager::loadFile(const std::filesystem::path& path, std::string* error) {
    std::error_code ec;
    const auto size = std::filesystem::file_size(path, ec);
    if (!ec && size > kMaxFileBytes) {
        if (error) {
            *error = path.string() + " is larger than " + std::to_string(kMaxFileBytes) + " bytes";
        }
        return false;
    }
    std::ifstream in(path);
    if (!in) {
        if (error) {
            *error = "could not read " + path.string();
        }
        return false;
    }
    std::string text(kMaxFileBytes + 1, '\0');
    in.read(text.data(), static_cast<std::streamsize>(text.size()));
    text.resize(static_cast<std::size_t>(in.gcount()));
    if (text.size() > kMaxFileBytes) {
        if (error) {
            *error = path.string() + " is larger than " + std::to_string(kMaxFileBytes) + " bytes";
        }
        return false;
    }
    const ParseResult parsed = parse(text);
    if (!parsed.ok) {
        if (error) {
            *error = parsed.error;
        }
        return false;
    }
    config_ = parsed.config;
    return true;
}

bool ConfigurationManager::saveFile(const std::filesystem::path& path, std::string* error) const {
    std::error_code ec;
    if (path.has_parent_path()) {
        std::filesystem::create_directories(path.parent_path(), ec);
        if (ec) {
            if (error) {
                *error = "could not create " + path.parent_path().string();
            }
            return false;
        }
    }
    const std::filesystem::path tmp = path.string() + ".tmp";
    {
        std::ofstream out(tmp, std::ios::trunc);
        if (!out) {
            if (error) {
                *error = "could not write " + tmp.string();
            }
            return false;
        }
        out << serialize(config_);
        out.flush();
        if (!out) {
            if (error) {
                *error = "could not write " + tmp.string();
            }
            std::filesystem::remove(tmp, ec);
            return false;
        }
    }
    // The car can cut power at any moment: the data must be on disk before the
    // rename makes it the live config.
    if (!syncPath(tmp)) {
        if (error) {
            *error = "could not flush " + tmp.string();
        }
        std::filesystem::remove(tmp, ec);
        return false;
    }
    std::filesystem::rename(tmp, path, ec);
    if (ec) {
        if (error) {
            *error = "could not replace " + path.string();
        }
        std::filesystem::remove(tmp, ec);
        return false;
    }
    syncPath(path.has_parent_path() ? path.parent_path() : std::filesystem::path("."));
    return true;
}

}  // namespace headunit
