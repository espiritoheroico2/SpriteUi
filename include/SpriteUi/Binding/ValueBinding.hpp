#pragma once

#include "SpriteUi/Core/Handle.hpp"
#include "SpriteUi/Core/Math.hpp"

#include <cmath>
#include <string>
#include <string_view>

namespace ToolEngine::SpriteUi::Binding {

// Reactive dirty-check binding for floating point values (Section 28 & Appendix F.3).
// Only triggers visual layout or property updates when value exceeds epsilon delta.
struct UIFloatBinding {
    UIHandle targetEntity{};
    float lastKnownValue = -1.0e9f;
    float threshold = 0.001f;

    [[nodiscard]] bool Update(float currentValue) noexcept {
        if (!IsFinite(currentValue)) return false;
        if (std::abs(currentValue - lastKnownValue) > threshold) {
            lastKnownValue = currentValue;
            return true;
        }
        return false;
    }
};

struct UIIntBinding {
    UIHandle targetEntity{};
    int lastKnownValue = -999999;

    [[nodiscard]] bool Update(int currentValue) noexcept {
        if (currentValue != lastKnownValue) {
            lastKnownValue = currentValue;
            return true;
        }
        return false;
    }
};

struct UIBoolBinding {
    UIHandle targetEntity{};
    bool lastKnownValue = false;
    bool initialized = false;

    [[nodiscard]] bool Update(bool currentValue) noexcept {
        if (!initialized || currentValue != lastKnownValue) {
            lastKnownValue = currentValue;
            initialized = true;
            return true;
        }
        return false;
    }
};

struct UIStringBinding {
    UIHandle targetEntity{};
    std::string lastKnownValue;

    [[nodiscard]] bool Update(std::string_view currentValue) {
        if (currentValue != lastKnownValue) {
            lastKnownValue = currentValue;
            return true;
        }
        return false;
    }
};

} // namespace ToolEngine::SpriteUi::Binding
