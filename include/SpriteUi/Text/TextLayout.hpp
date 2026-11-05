#pragma once

#include "SpriteUi/Core/Handle.hpp"
#include "SpriteUi/Core/Math.hpp"
#include "SpriteUi/Core/Transform2D.hpp"
#include "SpriteUi/Rendering/DrawList.hpp"
#include "SpriteUi/Text/FontMetrics.hpp"
#include "SpriteUi/Text/GlyphInstance.hpp"

#include <cstdint>
#include <span>
#include <vector>

namespace ToolEngine::SpriteUi::Text {

enum class TextAlignment : std::uint8_t {
    Left,
    Center,
    Right
};

enum class TextWrap : std::uint8_t {
    None,
    Word,
    Char
};

struct GlyphQuad {
    std::uint32_t glyphId = 0;
    std::uint32_t clusterByteOffset = 0;
    Rect bounds{};
    float uvRect[4]{0.0f, 0.0f, 0.0f, 0.0f};
    std::uint16_t atlasPage = 0;
};

struct TextLayoutConfig {
    float fontSize = 16.0f;
    float maxWidth = 0.0f; // <= 0.0f means unconstrained width
    float lineHeightMultiplier = 1.0f;
    TextAlignment alignment = TextAlignment::Left;
    TextWrap wrap = TextWrap::None;
};

struct TextLayoutResult {
    std::vector<GlyphQuad> quads;
    Vec2 size{0.0f, 0.0f};
    std::size_t lineCount = 0;
};

// Computes geometric layout and pixel quads for shaped HarfBuzz glyph instances.
[[nodiscard]] TextLayoutResult LayoutText(
    std::span<const GlyphInstance> shapedGlyphs,
    const FontMetrics& metrics,
    const TextLayoutConfig& config = {});

// Computes the caret position (x, y) for interactive cursor rendering.
[[nodiscard]] Vec2 GetCaretPosition(
    const TextLayoutResult& layout,
    std::uint32_t clusterByteOffset,
    const FontMetrics& metrics) noexcept;

// Finds the nearest cluster byte offset for a given local click position.
[[nodiscard]] std::uint32_t FindClusterAtPosition(
    const TextLayoutResult& layout,
    Vec2 localPos) noexcept;

// Converts layout quads into standard UIRenderItem entries for GPU extraction.
[[nodiscard]] std::vector<Rendering::UIRenderItem> GenerateTextRenderItems(
    const TextLayoutResult& layout,
    const UILayoutRect& rect,
    Vec4 colorPremultiplied,
    std::uint32_t atlasTextureSlice,
    std::uint32_t clipIndex = Rendering::NoClipIndex,
    float zOrder = 0.0f,
    std::uint64_t basePaintOrder = 0,
    UIHandle owner = {}); // Handle valido e obrigatorio para o RenderExtractor aceitar os itens.

} // namespace ToolEngine::SpriteUi::Text

