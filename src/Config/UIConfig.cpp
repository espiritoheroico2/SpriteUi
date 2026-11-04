#include "SpriteUi/Config/UIConfig.hpp"

#include <charconv>
#include <sstream>
#include <string>

namespace ToolEngine::SpriteUi::Config {

namespace {

void SkipWhitespace(std::string_view s, std::size_t& pos) {
    while (pos < s.size() && (s[pos] == ' ' || s[pos] == '\t' || s[pos] == '\r' || s[pos] == '\n')) {
        ++pos;
    }
}

bool MatchString(std::string_view s, std::size_t& pos, std::string_view expected) {
    SkipWhitespace(s, pos);
    if (s.substr(pos, expected.size()) == expected) {
        pos += expected.size();
        return true;
    }
    return false;
}

std::optional<std::string> ParseStringValue(std::string_view s, std::size_t& pos) {
    SkipWhitespace(s, pos);
    if (pos >= s.size() || s[pos] != '"') return std::nullopt;
    ++pos;
    const std::size_t start = pos;
    while (pos < s.size() && s[pos] != '"') {
        ++pos;
    }
    if (pos >= s.size()) return std::nullopt;
    std::string result(s.substr(start, pos - start));
    ++pos;
    return result;
}

std::optional<float> ParseFloatValue(std::string_view s, std::size_t& pos) {
    SkipWhitespace(s, pos);
    const std::size_t start = pos;
    if (pos < s.size() && (s[pos] == '-' || s[pos] == '+')) ++pos;
    while (pos < s.size() && ((s[pos] >= '0' && s[pos] <= '9') || s[pos] == '.' || s[pos] == 'e' || s[pos] == 'E')) {
        ++pos;
    }
    if (start == pos) return std::nullopt;
    float val = 0.0f;
    auto [ptr, ec] = std::from_chars(s.data() + start, s.data() + pos, val);
    if (ec != std::errc()) return std::nullopt;
    return val;
}

std::optional<int> ParseIntValue(std::string_view s, std::size_t& pos) {
    SkipWhitespace(s, pos);
    const std::size_t start = pos;
    if (pos < s.size() && (s[pos] == '-' || s[pos] == '+')) ++pos;
    while (pos < s.size() && (s[pos] >= '0' && s[pos] <= '9')) {
        ++pos;
    }
    if (start == pos) return std::nullopt;
    int val = 0;
    auto [ptr, ec] = std::from_chars(s.data() + start, s.data() + pos, val);
    if (ec != std::errc()) return std::nullopt;
    return val;
}

std::optional<bool> ParseBoolValue(std::string_view s, std::size_t& pos) {
    SkipWhitespace(s, pos);
    if (s.substr(pos, 4) == "true") {
        pos += 4;
        return true;
    }
    if (s.substr(pos, 5) == "false") {
        pos += 5;
        return false;
    }
    return std::nullopt;
}

} // namespace

bool UIConfigManager::ValidateAndStage(
    std::string_view jsonText,
    UISettingsConfig& outStaged)
{
    if (jsonText.empty()) return false;

    UISettingsConfig staged;
    bool foundSchema = false;

    std::size_t pos = 0;
    SkipWhitespace(jsonText, pos);
    if (pos >= jsonText.size() || jsonText[pos] != '{') return false;
    ++pos;

    while (pos < jsonText.size()) {
        SkipWhitespace(jsonText, pos);
        if (pos < jsonText.size() && jsonText[pos] == '}') {
            ++pos;
            break;
        }

        auto keyOpt = ParseStringValue(jsonText, pos);
        if (!keyOpt) return false;
        SkipWhitespace(jsonText, pos);
        if (pos >= jsonText.size() || jsonText[pos] != ':') return false;
        ++pos;

        const std::string& key = *keyOpt;
        if (key == "schemaVersion") {
            auto val = ParseIntValue(jsonText, pos);
            if (!val || *val != static_cast<int>(CurrentSchemaVersion)) {
                return false; // Rejection: wrong or missing schemaVersion
            }
            staged.schemaVersion = static_cast<std::uint32_t>(*val);
            foundSchema = true;
        } else if (key == "globalScale") {
            auto val = ParseFloatValue(jsonText, pos);
            staged.globalScale = ReadFiniteFloat(val.value_or(1.0f), 1.0f, 0.1f, 10.0f);
        } else if (key == "activeTheme") {
            auto val = ParseIntValue(jsonText, pos);
            staged.activeTheme = val.value_or(0);
        } else if (key == "enableVSync") {
            auto val = ParseBoolValue(jsonText, pos);
            staged.enableVSync = val.value_or(true);
        } else if (key == "windows") {
            SkipWhitespace(jsonText, pos);
            if (pos >= jsonText.size() || jsonText[pos] != '[') return false;
            ++pos;

            while (pos < jsonText.size()) {
                SkipWhitespace(jsonText, pos);
                if (pos < jsonText.size() && jsonText[pos] == ']') {
                    ++pos;
                    break;
                }
                if (pos < jsonText.size() && jsonText[pos] == '{') {
                    ++pos;
                    UIWindowConfig win;
                    while (pos < jsonText.size()) {
                        SkipWhitespace(jsonText, pos);
                        if (pos < jsonText.size() && jsonText[pos] == '}') {
                            ++pos;
                            break;
                        }
                        auto wKeyOpt = ParseStringValue(jsonText, pos);
                        if (!wKeyOpt) return false;
                        SkipWhitespace(jsonText, pos);
                        if (pos >= jsonText.size() || jsonText[pos] != ':') return false;
                        ++pos;

                        const std::string& wKey = *wKeyOpt;
                        if (wKey == "name") {
                            win.name = ParseStringValue(jsonText, pos).value_or("");
                        } else if (wKey == "offsetMinX") {
                            auto f = ParseFloatValue(jsonText, pos);
                            win.offsetMin.x = ReadFiniteFloat(f.value_or(0.0f), 0.0f, -100000.0f, 100000.0f);
                        } else if (wKey == "offsetMinY") {
                            auto f = ParseFloatValue(jsonText, pos);
                            win.offsetMin.y = ReadFiniteFloat(f.value_or(0.0f), 0.0f, -100000.0f, 100000.0f);
                        } else if (wKey == "offsetMaxX") {
                            auto f = ParseFloatValue(jsonText, pos);
                            win.offsetMax.x = ReadFiniteFloat(f.value_or(150.0f), 150.0f, -100000.0f, 100000.0f);
                        } else if (wKey == "offsetMaxY") {
                            auto f = ParseFloatValue(jsonText, pos);
                            win.offsetMax.y = ReadFiniteFloat(f.value_or(100.0f), 100.0f, -100000.0f, 100000.0f);
                        } else if (wKey == "isMinimized") {
                            win.isMinimized = ParseBoolValue(jsonText, pos).value_or(false);
                        } else {
                            // Skip unknown value
                            ParseStringValue(jsonText, pos);
                        }

                        SkipWhitespace(jsonText, pos);
                        if (pos < jsonText.size() && jsonText[pos] == ',') ++pos;
                    }
                    staged.windows.push_back(std::move(win));
                }
                SkipWhitespace(jsonText, pos);
                if (pos < jsonText.size() && jsonText[pos] == ',') ++pos;
            }
        }

        SkipWhitespace(jsonText, pos);
        if (pos < jsonText.size() && jsonText[pos] == ',') ++pos;
    }

    if (!foundSchema || !staged.IsValid()) {
        return false;
    }

    outStaged = std::move(staged);
    return true;
}

void UIConfigManager::Commit(UISettingsConfig& target, const UISettingsConfig& staged) noexcept {
    if (staged.IsValid()) {
        target = staged;
    }
}

std::string UIConfigManager::Serialize(const UISettingsConfig& config) {
    std::ostringstream ss;
    ss << "{\n";
    ss << "  \"schemaVersion\": " << config.schemaVersion << ",\n";
    ss << "  \"globalScale\": " << config.globalScale << ",\n";
    ss << "  \"activeTheme\": " << config.activeTheme << ",\n";
    ss << "  \"enableVSync\": " << (config.enableVSync ? "true" : "false") << ",\n";
    ss << "  \"windows\": [\n";

    for (std::size_t i = 0; i < config.windows.size(); ++i) {
        const auto& win = config.windows[i];
        ss << "    {\n";
        ss << "      \"name\": \"" << win.name << "\",\n";
        ss << "      \"offsetMinX\": " << win.offsetMin.x << ",\n";
        ss << "      \"offsetMinY\": " << win.offsetMin.y << ",\n";
        ss << "      \"offsetMaxX\": " << win.offsetMax.x << ",\n";
        ss << "      \"offsetMaxY\": " << win.offsetMax.y << ",\n";
        ss << "      \"isMinimized\": " << (win.isMinimized ? "true" : "false") << "\n";
        ss << "    }" << (i + 1 < config.windows.size() ? ",\n" : "\n");
    }

    ss << "  ]\n";
    ss << "}\n";
    return ss.str();
}

} // namespace ToolEngine::SpriteUi::Config

