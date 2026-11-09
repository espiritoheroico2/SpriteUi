#include <SpriteUi/SpriteUi.hpp>

#include <vector>

int main() {
    ToolEngine::SpriteUi::Rendering::RenderExtractor extractor;
    const std::vector<ToolEngine::SpriteUi::Rendering::UIRenderItem> items;
    const auto packet = extractor.Build(1, items);
    return ToolEngine::SpriteUi::Rendering::IsFramePacketValid(packet) ? 0 : 1;
}
