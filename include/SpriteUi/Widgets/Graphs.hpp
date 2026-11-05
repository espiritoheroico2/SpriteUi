#pragma once

#include "SpriteUi/Core/Math.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace ToolEngine::SpriteUi::Widgets {

// Oscilloscope and scrolling telemetry graph (Section 29 & 30.1).
// Stores a fixed circular ring buffer of 256 samples with O(1) ingestion.
struct UIOscilloscopeGraph {
    static constexpr std::size_t HistoryCapacity = 256;

    float valueHistory[HistoryCapacity]{};
    std::size_t headIndex = 0;
    std::size_t sampleCount = 0;

    Vec4 lineColor{0.0f, 1.0f, 0.0f, 1.0f};
    float thickness = 2.0f;
    float scrollSpeed = 50.0f;

    void AddSample(float value) noexcept;
    void Clear() noexcept;
    [[nodiscard]] float GetLatestSample() const noexcept;

    // Writes samples in chronological order (oldest to newest) into outBuffer
    void GetOrderedSamples(std::span<float> outBuffer) const noexcept;

    // Normalizes samples in [0, 1] range. If minVal == maxVal, automatically computes min/max
    void GetNormalizedSamples(std::span<float> outBuffer, float minVal = 0.0f, float maxVal = 0.0f) const noexcept;
};

// Line plot for static or dynamic 2D vector series (Section 29).
struct UILinePlot {
    std::vector<Vec2> points;
    Vec4 lineColor{0.2f, 0.8f, 1.0f, 1.0f};
    float thickness = 2.0f;

    void AddPoint(Vec2 point);
    void Clear() noexcept { points.clear(); }
    [[nodiscard]] Rect ComputeBounds() const noexcept;
};

} // namespace ToolEngine::SpriteUi::Widgets

