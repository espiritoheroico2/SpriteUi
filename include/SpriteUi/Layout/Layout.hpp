#pragma once
#include "SpriteUi/Core/Math.hpp"
#include <cstdint>
#include <limits>
#include <vector>
namespace ToolEngine::SpriteUi {
enum class LayoutDirection : uint8_t { Row, Column };
enum class SizeMode : uint8_t { Fixed, Percent, Auto, Flex };
struct Length { SizeMode mode=SizeMode::Auto; float value=0.0f; };
struct Edges { float left=0, top=0, right=0, bottom=0; };
struct FlexItem {
    Length width{}, height{};
    float grow=0.0f, shrink=1.0f;
    Edges margin{}, padding{};
    float gap=0.0f;
    LayoutDirection direction=LayoutDirection::Row;
    Vec2 minSize{0,0};
    Vec2 maxSize{std::numeric_limits<float>::infinity(),std::numeric_limits<float>::infinity()};
};
struct LayoutNode {
    FlexItem style{};
    Rect rect{};
    Vec2 measured{};
    std::vector<LayoutNode> children;
};
struct LayoutStats { uint32_t measured=0, arranged=0; };
class LayoutEngine {
public:
    Vec2 Measure(LayoutNode& node, Vec2 available, LayoutStats* stats=nullptr) const;
    void Arrange(LayoutNode& node, Rect bounds, LayoutStats* stats=nullptr) const;
private:
    Vec2 MeasureImpl(LayoutNode&, Vec2, LayoutStats*) const;
    void ArrangeImpl(LayoutNode&, Rect, LayoutStats*) const;
};
} // namespace
