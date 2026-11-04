#include "SpriteUi/Widgets/Graphs.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace ToolEngine::SpriteUi::Widgets {

void UIOscilloscopeGraph::AddSample(float value) noexcept {
    if (!IsFinite(value)) value = 0.0f;
    valueHistory[headIndex] = value;
    headIndex = (headIndex + 1) % HistoryCapacity;
    if (sampleCount < HistoryCapacity) {
        ++sampleCount;
    }
}

void UIOscilloscopeGraph::Clear() noexcept {
    for (std::size_t i = 0; i < HistoryCapacity; ++i) {
        valueHistory[i] = 0.0f;
    }
    headIndex = 0;
    sampleCount = 0;
}

float UIOscilloscopeGraph::GetLatestSample() const noexcept {
    if (sampleCount == 0) return 0.0f;
    const std::size_t latestIdx = (headIndex == 0) ? (HistoryCapacity - 1) : (headIndex - 1);
    return valueHistory[latestIdx];
}

void UIOscilloscopeGraph::GetOrderedSamples(std::span<float> outBuffer) const noexcept {
    if (outBuffer.empty() || sampleCount == 0) return;

    const std::size_t count = std::min(outBuffer.size(), sampleCount);
    const std::size_t start = (sampleCount < HistoryCapacity)
        ? 0
        : headIndex;

    for (std::size_t i = 0; i < count; ++i) {
        const std::size_t ringIdx = (start + i) % HistoryCapacity;
        outBuffer[i] = valueHistory[ringIdx];
    }
}

void UIOscilloscopeGraph::GetNormalizedSamples(std::span<float> outBuffer, float minVal, float maxVal) const noexcept {
    if (outBuffer.empty() || sampleCount == 0) return;

    GetOrderedSamples(outBuffer);
    const std::size_t count = std::min(outBuffer.size(), sampleCount);

    float lo = minVal;
    float hi = maxVal;

    if (lo >= hi) {
        lo = std::numeric_limits<float>::infinity();
        hi = -std::numeric_limits<float>::infinity();
        for (std::size_t i = 0; i < count; ++i) {
            lo = std::min(lo, outBuffer[i]);
            hi = std::max(hi, outBuffer[i]);
        }
        if (lo >= hi) {
            hi = lo + 1.0f;
        }
    }

    const float range = hi - lo;
    for (std::size_t i = 0; i < count; ++i) {
        outBuffer[i] = std::clamp((outBuffer[i] - lo) / range, 0.0f, 1.0f);
    }
}

void UILinePlot::AddPoint(Vec2 point) {
    if (IsFinite(point)) {
        points.push_back(point);
    }
}

Rect UILinePlot::ComputeBounds() const noexcept {
    if (points.empty()) return Rect{};

    Vec2 minPt{std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity()};
    Vec2 maxPt{-std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity()};

    for (const auto& pt : points) {
        minPt.x = std::min(minPt.x, pt.x);
        minPt.y = std::min(minPt.y, pt.y);
        maxPt.x = std::max(maxPt.x, pt.x);
        maxPt.y = std::max(maxPt.y, pt.y);
    }

    return Rect{minPt, {std::max(0.0f, maxPt.x - minPt.x), std::max(0.0f, maxPt.y - minPt.y)}};
}

} // namespace ToolEngine::SpriteUi::Widgets
