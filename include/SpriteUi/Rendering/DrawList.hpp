#pragma once

#include "SpriteUi/Core/Handle.hpp"
#include "SpriteUi/Core/Transform2D.hpp"
#include "SpriteUi/Rendering/SpriteInstanceGPU.hpp"

#include <cstdint>
#include <span>
#include <vector>

namespace ToolEngine::SpriteUi::Rendering {

inline constexpr std::uint32_t WhiteTextureSlice = 0;

struct UIRenderItem {
    UIHandle handle{};
    UILayoutRect layout{};
    UIRectTransform transform{};

    float uvRect[4]{0.0f, 0.0f, 1.0f, 1.0f};
    Vec4 tintPremultipliedLinear{1.0f, 1.0f, 1.0f, 1.0f};

    std::uint32_t textureSlice = WhiteTextureSlice;
    std::uint32_t materialIndex = 0;
    std::uint32_t clipIndex = 0xFFFFFFFFu;
    std::uint32_t flags = 0;

    std::int32_t layer = 0;
    float zOrder = 0.0f;
    std::uint64_t paintOrder = 0;
    bool visible = true;
};

struct ClipRectGPU {
    float minX = 0.0f;
    float minY = 0.0f;
    float maxX = 0.0f;
    float maxY = 0.0f;
};

struct DrawBatch {
    std::uint32_t firstInstance = 0;
    std::uint32_t instanceCount = 0;
    std::uint32_t textureSlice = WhiteTextureSlice;
    std::uint32_t materialIndex = 0;
    std::uint32_t clipIndex = 0xFFFFFFFFu;
    std::uint32_t flags = 0;
};

struct FramePacket {
    std::uint64_t frameId = 0;
    std::vector<SpriteInstanceGPU> instances;
    std::vector<DrawBatch> batches;
    std::vector<ClipRectGPU> clipRects;
};

struct ExtractionStats {
    std::uint32_t submittedItems = 0;
    std::uint32_t skippedInvisible = 0;
    std::uint32_t skippedInvalid = 0;
    std::uint32_t batchCount = 0;
};

[[nodiscard]] bool IsFramePacketValid(const FramePacket& packet) noexcept;

class RenderExtractor {
public:
    [[nodiscard]] FramePacket Build(
        std::uint64_t frameId,
        const std::vector<UIRenderItem>& items,
        ExtractionStats* stats = nullptr,
        std::span<const ClipRectGPU> clipRects = {}) const;
};

} // namespace ToolEngine::SpriteUi::Rendering
