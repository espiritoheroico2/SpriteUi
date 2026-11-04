#pragma once

#include "SpriteUi/Core/Math.hpp"

#include <algorithm>
#include <cmath>

namespace ToolEngine::SpriteUi::Widgets {

struct UISliderComponent {
    float value = 0.5f; // Value in domain [minValue, maxValue].
    float minValue = 0.0f;
    float maxValue = 1.0f;
    float step = 0.0f;  // Zero means continuous.
    bool isDragging = false;

    [[nodiscard]] bool SetValue(float candidate) noexcept {
        if (!IsFinite(candidate) || !IsFinite(minValue) || !IsFinite(maxValue) ||
            maxValue < minValue || !IsFinite(step) || step < 0.0f) {
            return false;
        }
        float next = std::clamp(candidate, minValue, maxValue);
        if (step > 0.0f) {
            next = minValue + std::round((next - minValue) / step) * step;
            next = std::clamp(next, minValue, maxValue);
        }
        value = next;
        return true;
    }

    [[nodiscard]] bool SetFromPointer(float localX, float viewportWidth) noexcept {
        if (!IsFinite(localX) || !IsFinite(viewportWidth) || viewportWidth <= 1.0e-6f ||
            !IsFinite(minValue) || !IsFinite(maxValue) || maxValue < minValue) {
            return false;
        }
        const float normalized = Clamp01(localX / viewportWidth);
        return SetValue(minValue + normalized * (maxValue - minValue));
    }
};

struct UIScrollViewComponent {
    Vec2 scrollOffset{};
    Vec2 contentSize{};
    Vec2 viewportSize{};
    bool allowScrollX = false;
    bool allowScrollY = true;

    void ClampScroll() noexcept {
        if (!IsFinite(scrollOffset)) scrollOffset = {};
        if (!IsFinite(contentSize)) contentSize = {};
        if (!IsFinite(viewportSize)) viewportSize = {};

        const float maxX = std::max(0.0f, contentSize.x - viewportSize.x);
        const float maxY = std::max(0.0f, contentSize.y - viewportSize.y);
        scrollOffset.x = allowScrollX ? std::clamp(scrollOffset.x, 0.0f, maxX) : 0.0f;
        scrollOffset.y = allowScrollY ? std::clamp(scrollOffset.y, 0.0f, maxY) : 0.0f;
    }

    void ScrollBy(Vec2 delta) noexcept {
        if (!IsFinite(delta)) return;
        scrollOffset += delta;
        ClampScroll();
    }
};

struct UIButtonState {
    bool hovered = false;
    bool pressed = false;
    bool disabled = false;
};

} // namespace ToolEngine::SpriteUi::Widgets
