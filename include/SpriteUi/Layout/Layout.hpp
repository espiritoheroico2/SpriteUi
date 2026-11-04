#pragma once

#include "SpriteUi/Core/Math.hpp"

#include <cstdint>
#include <limits>
#include <vector>

namespace ToolEngine::SpriteUi {

enum class LayoutDirection : std::uint8_t {
    Row,
    Column
};

enum class AlignItems : std::uint8_t {
    Stretch,
    Start,
    Center,
    End
};

enum class JustifyContent : std::uint8_t {
    FlexStart,
    Center,
    FlexEnd,
    SpaceBetween,
    SpaceAround,
    SpaceEvenly
};

enum class SizeMode : std::uint8_t {
    Fixed,
    Percent,
    Auto,
    Flex
};

struct Length {
    SizeMode mode = SizeMode::Auto;
    float value = 0.0f;
};

struct Edges {
    float left = 0.0f;
    float top = 0.0f;
    float right = 0.0f;
    float bottom = 0.0f;
};

struct FlexItem {
    Length width{};
    Length height{};
    float grow = 0.0f;
    float shrink = 1.0f;
    Edges margin{};
    Edges padding{};
    float gap = 0.0f;
    LayoutDirection direction = LayoutDirection::Row;
    AlignItems alignItems = AlignItems::Stretch;
    JustifyContent justifyContent = JustifyContent::FlexStart;
    Vec2 minSize{0.0f, 0.0f};
    Vec2 maxSize{
        std::numeric_limits<float>::infinity(),
        std::numeric_limits<float>::infinity()
    };
};

struct LayoutNode {
    FlexItem style{};
    Rect rect{};
    Vec2 measured{};
    std::vector<LayoutNode> children;
};

struct LayoutStats {
    std::uint32_t measured = 0;
    std::uint32_t arranged = 0;
};

class LayoutEngine {
public:
    [[nodiscard]] Vec2 Measure(
        LayoutNode& node,
        Vec2 available,
        LayoutStats* stats = nullptr) const;

    void Arrange(
        LayoutNode& node,
        Rect bounds,
        LayoutStats* stats = nullptr) const;

private:
    [[nodiscard]] Vec2 MeasureImpl(
        LayoutNode& node,
        Vec2 available,
        LayoutStats* stats) const;

    void ArrangeImpl(
        LayoutNode& node,
        Rect bounds,
        LayoutStats* stats) const;
};

} // namespace ToolEngine::SpriteUi
