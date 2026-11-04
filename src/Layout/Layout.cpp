#include "SpriteUi/Layout/Layout.hpp"

#include <algorithm>
#include <cmath>

namespace ToolEngine::SpriteUi {

namespace {

float ResolveLength(Length length, float available, float measured) noexcept {
    switch (length.mode) {
    case SizeMode::Fixed:
        return std::max(0.0f, length.value);
    case SizeMode::Percent:
        return std::max(0.0f, available * length.value);
    case SizeMode::Flex:
    case SizeMode::Auto:
    default:
        return std::max(0.0f, measured);
    }
}

float ClampSize(float value, float minVal, float maxVal) noexcept {
    if (!std::isfinite(value)) value = 0.0f;
    const float lo = std::max(0.0f, minVal);
    const float hi = std::max(lo, maxVal);
    return std::clamp(value, lo, hi);
}

} // namespace

Vec2 LayoutEngine::Measure(LayoutNode& node, Vec2 available, LayoutStats* stats) const {
    return MeasureImpl(node, available, stats);
}

Vec2 LayoutEngine::MeasureImpl(LayoutNode& node, Vec2 available, LayoutStats* stats) const {
    if (stats) ++stats->measured;

    float mainAxis = 0.0f;
    float crossAxis = 0.0f;
    const bool isRow = (node.style.direction == LayoutDirection::Row);

    for (auto& child : node.children) {
        const Vec2 childMeasured = MeasureImpl(child, available, stats);
        const float childMain = isRow ? childMeasured.x : childMeasured.y;
        const float childCross = isRow ? childMeasured.y : childMeasured.x;

        const float mainMargins = isRow
            ? child.style.margin.left + child.style.margin.right
            : child.style.margin.top + child.style.margin.bottom;
        const float crossMargins = isRow
            ? child.style.margin.top + child.style.margin.bottom
            : child.style.margin.left + child.style.margin.right;

        mainAxis += childMain + mainMargins;
        crossAxis = std::max(crossAxis, childCross + crossMargins);
    }

    if (!node.children.empty()) {
        mainAxis += node.style.gap * static_cast<float>(node.children.size() - 1);
    }

    Vec2 content = isRow ? Vec2{mainAxis, crossAxis} : Vec2{crossAxis, mainAxis};
    content.x += node.style.padding.left + node.style.padding.right;
    content.y += node.style.padding.top + node.style.padding.bottom;

    node.measured = {
        ClampSize(ResolveLength(node.style.width, available.x, content.x),
                  node.style.minSize.x, node.style.maxSize.x),
        ClampSize(ResolveLength(node.style.height, available.y, content.y),
                  node.style.minSize.y, node.style.maxSize.y)
    };

    return node.measured;
}

void LayoutEngine::Arrange(LayoutNode& node, Rect bounds, LayoutStats* stats) const {
    ArrangeImpl(node, bounds, stats);
}

void LayoutEngine::ArrangeImpl(LayoutNode& node, Rect bounds, LayoutStats* stats) const {
    if (stats) ++stats->arranged;
    node.rect = bounds;
    if (node.children.empty()) return;

    const bool isRow = (node.style.direction == LayoutDirection::Row);

    const float padBeforeMain = isRow ? node.style.padding.left : node.style.padding.top;
    const float padAfterMain  = isRow ? node.style.padding.right : node.style.padding.bottom;
    const float padBeforeCross = isRow ? node.style.padding.top : node.style.padding.left;
    const float padAfterCross  = isRow ? node.style.padding.bottom : node.style.padding.right;

    float innerMain = (isRow ? bounds.size.x : bounds.size.y) - (padBeforeMain + padAfterMain);
    float innerCross = (isRow ? bounds.size.y : bounds.size.x) - (padBeforeCross + padAfterCross);
    innerMain = std::max(0.0f, innerMain);
    innerCross = std::max(0.0f, innerCross);

    float fixedMain = node.style.gap * static_cast<float>(node.children.size() - 1);
    float totalGrow = 0.0f;
    float totalShrink = 0.0f;

    std::vector<float> bases;
    bases.reserve(node.children.size());

    for (auto& child : node.children) {
        float base = isRow ? child.measured.x : child.measured.y;
        float margin = isRow
            ? child.style.margin.left + child.style.margin.right
            : child.style.margin.top + child.style.margin.bottom;
        base = std::max(0.0f, base);
        bases.push_back(base);
        fixedMain += base + margin;
        totalGrow += std::max(0.0f, child.style.grow);
        totalShrink += std::max(0.0f, child.style.shrink) * base;
    }

    const float free = innerMain - fixedMain;

    float initialOffset = 0.0f;
    float extraGap = 0.0f;

    // Apply JustifyContent when free space remains and items don't grow
    if (free > 0.0f && totalGrow <= 0.0f) {
        switch (node.style.justifyContent) {
        case JustifyContent::Center:
            initialOffset = free * 0.5f;
            break;
        case JustifyContent::FlexEnd:
            initialOffset = free;
            break;
        case JustifyContent::SpaceBetween:
            if (node.children.size() > 1) {
                extraGap = free / static_cast<float>(node.children.size() - 1);
            }
            break;
        case JustifyContent::SpaceAround:
            if (!node.children.empty()) {
                const float share = free / static_cast<float>(node.children.size());
                initialOffset = share * 0.5f;
                extraGap = share;
            }
            break;
        case JustifyContent::SpaceEvenly:
            if (!node.children.empty()) {
                const float step = free / static_cast<float>(node.children.size() + 1);
                initialOffset = step;
                extraGap = step;
            }
            break;
        case JustifyContent::FlexStart:
        default:
            break;
        }
    }

    float cursor = (isRow ? bounds.position.x : bounds.position.y) + padBeforeMain + initialOffset;

    for (std::size_t i = 0; i < node.children.size(); ++i) {
        auto& child = node.children[i];

        const float beforeMarginMain = isRow ? child.style.margin.left : child.style.margin.top;
        const float afterMarginMain  = isRow ? child.style.margin.right : child.style.margin.bottom;

        float mainSize = bases[i];
        if (free > 0.0f && totalGrow > 0.0f) {
            mainSize += free * std::max(0.0f, child.style.grow) / totalGrow;
        } else if (free < 0.0f && totalShrink > 0.0f) {
            mainSize += free * (std::max(0.0f, child.style.shrink) * bases[i]) / totalShrink;
        }
        mainSize = std::max(0.0f, mainSize);

        const float beforeMarginCross = isRow ? child.style.margin.top : child.style.margin.left;
        const float afterMarginCross  = isRow ? child.style.margin.bottom : child.style.margin.right;
        const float crossAvailable = std::max(0.0f, innerCross - beforeMarginCross - afterMarginCross);

        const float measuredCross = isRow ? child.measured.y : child.measured.x;
        const float minCross = isRow ? child.style.minSize.y : child.style.minSize.x;
        const float maxCross = isRow ? child.style.maxSize.y : child.style.maxSize.x;

        float crossSize = (child.style.alignItems == AlignItems::Stretch)
            ? crossAvailable
            : ClampSize(measuredCross, minCross, maxCross);
        crossSize = std::min(crossSize, crossAvailable);

        float crossOffset = beforeMarginCross;
        if (child.style.alignItems == AlignItems::Center) {
            crossOffset += std::max(0.0f, crossAvailable - crossSize) * 0.5f;
        } else if (child.style.alignItems == AlignItems::End) {
            crossOffset += std::max(0.0f, crossAvailable - crossSize);
        }

        cursor += beforeMarginMain;

        Rect childBounds;
        if (isRow) {
            childBounds = {
                {cursor, bounds.position.y + padBeforeCross + crossOffset},
                {mainSize, crossSize}
            };
        } else {
            childBounds = {
                {bounds.position.x + padBeforeCross + crossOffset, cursor},
                {crossSize, mainSize}
            };
        }

        ArrangeImpl(child, childBounds, stats);
        cursor += mainSize + afterMarginMain + node.style.gap + extraGap;
    }
}

} // namespace ToolEngine::SpriteUi
