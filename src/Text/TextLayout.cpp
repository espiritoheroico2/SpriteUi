#include "SpriteUi/Text/TextLayout.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace ToolEngine::SpriteUi::Text {

namespace {

struct LineInfo {
    std::size_t startQuad = 0;
    std::size_t endQuad = 0;
    float width = 0.0f;
    float y = 0.0f;
    float height = 0.0f;
};

} // namespace

TextLayoutResult LayoutText(
    std::span<const GlyphInstance> shapedGlyphs,
    const FontMetrics& metrics,
    const TextLayoutConfig& config)
{
    TextLayoutResult result;
    if (shapedGlyphs.empty() || !metrics.IsValid()) {
        return result;
    }

    const float fontSize = (config.fontSize > 0.0f && IsFinite(config.fontSize))
        ? config.fontSize
        : metrics.fontSize;
    const float fontScale = (metrics.unitsPerEm > 0.0f) ? (fontSize / metrics.unitsPerEm) : 1.0f;
    const float lineHeight = metrics.GetLineHeight() * fontScale * std::max(0.1f, config.lineHeightMultiplier);
    const float ascent = metrics.ascent * fontScale;

    std::vector<LineInfo> lines;
    LineInfo currentLine;
    currentLine.startQuad = 0;
    currentLine.y = 0.0f;
    currentLine.height = lineHeight;

    float penX = 0.0f;
    float penY = 0.0f;
    float maxLineWidth = 0.0f;

    for (const auto& glyph : shapedGlyphs) {
        const float xAdv = glyph.xAdvance * fontScale;
        const float yAdv = glyph.yAdvance * fontScale;

        // Wrap check
        if (config.maxWidth > 0.0f && config.wrap != TextWrap::None) {
            if (penX + xAdv > config.maxWidth && penX > 0.0f) {
                // End current line
                currentLine.endQuad = result.quads.size();
                currentLine.width = penX;
                maxLineWidth = std::max(maxLineWidth, penX);
                lines.push_back(currentLine);

                // Start new line
                penX = 0.0f;
                penY += lineHeight;
                currentLine.startQuad = result.quads.size();
                currentLine.y = penY;
                currentLine.height = lineHeight;
            }
        }

        const float quadW = (glyph.planeBounds[2] - glyph.planeBounds[0]) * fontScale;
        const float quadH = (glyph.planeBounds[3] - glyph.planeBounds[1]) * fontScale;

        GlyphQuad quad;
        quad.glyphId = glyph.glyphId;
        quad.clusterByteOffset = glyph.clusterByteOffset;
        quad.atlasPage = glyph.atlasPage;

        for (int k = 0; k < 4; ++k) {
            quad.uvRect[k] = glyph.uvRect[k];
        }

        if (quadW > 0.0f && quadH > 0.0f) {
            const float quadX = penX + glyph.xOffset * fontScale + glyph.planeBounds[0] * fontScale;
            // Baseline is at penY + ascent; planeBounds[3] is distance above baseline
            const float quadY = penY + ascent - glyph.planeBounds[3] * fontScale;
            quad.bounds = {{quadX, quadY}, {quadW, quadH}};
        } else {
            quad.bounds = {{penX, penY}, {0.0f, 0.0f}};
        }

        result.quads.push_back(quad);

        penX += xAdv;
        penY += yAdv;
    }

    currentLine.endQuad = result.quads.size();
    currentLine.width = penX;
    maxLineWidth = std::max(maxLineWidth, penX);
    lines.push_back(currentLine);

    // Apply alignment (Center, Right)
    const float containerWidth = (config.maxWidth > 0.0f) ? config.maxWidth : maxLineWidth;
    if (config.alignment != TextAlignment::Left && containerWidth > 0.0f) {
        for (const auto& line : lines) {
            float shiftX = 0.0f;
            if (config.alignment == TextAlignment::Center) {
                shiftX = std::max(0.0f, containerWidth - line.width) * 0.5f;
            } else if (config.alignment == TextAlignment::Right) {
                shiftX = std::max(0.0f, containerWidth - line.width);
            }

            if (shiftX > 0.0f) {
                for (std::size_t i = line.startQuad; i < line.endQuad; ++i) {
                    result.quads[i].bounds.position.x += shiftX;
                }
            }
        }
    }

    result.size = {containerWidth, penY + lineHeight};
    result.lineCount = lines.size();
    return result;
}

Vec2 GetCaretPosition(
    const TextLayoutResult& layout,
    std::uint32_t clusterByteOffset,
    [[maybe_unused]] const FontMetrics& metrics) noexcept
{
    if (layout.quads.empty()) {
        return {0.0f, 0.0f};
    }

    for (std::size_t i = 0; i < layout.quads.size(); ++i) {
        if (layout.quads[i].clusterByteOffset >= clusterByteOffset) {
            return {layout.quads[i].bounds.position.x, layout.quads[i].bounds.position.y};
        }
    }

    // After the last glyph
    const auto& last = layout.quads.back();
    return {last.bounds.position.x + last.bounds.size.x, last.bounds.position.y};
}

std::uint32_t FindClusterAtPosition(
    const TextLayoutResult& layout,
    Vec2 localPos) noexcept
{
    if (layout.quads.empty()) {
        return 0;
    }

    std::uint32_t closestCluster = layout.quads.front().clusterByteOffset;
    float closestDistSq = std::numeric_limits<float>::infinity();

    for (const auto& quad : layout.quads) {
        const Vec2 center = quad.bounds.position + quad.bounds.size * 0.5f;
        const float dx = center.x - localPos.x;
        const float dy = center.y - localPos.y;
        const float distSq = dx * dx + dy * dy;
        if (distSq < closestDistSq) {
            closestDistSq = distSq;
            closestCluster = quad.clusterByteOffset;
        }
    }

    return closestCluster;
}

std::vector<Rendering::UIRenderItem> GenerateTextRenderItems(
    const TextLayoutResult& layout,
    const UILayoutRect& rect,
    Vec4 colorPremultiplied,
    std::uint32_t atlasTextureSlice,
    std::uint32_t clipIndex,
    float zOrder,
    std::uint64_t basePaintOrder)
{
    std::vector<Rendering::UIRenderItem> items;
    items.reserve(layout.quads.size());

    std::uint64_t currentPaint = basePaintOrder;
    for (const auto& quad : layout.quads) {
        if (quad.bounds.size.x <= 0.0f || quad.bounds.size.y <= 0.0f) {
            continue; // Skip non-printable or whitespace quads
        }

        Rendering::UIRenderItem item;
        item.layout.resolvedPosition = rect.resolvedPosition + quad.bounds.position;
        item.layout.resolvedSize = quad.bounds.size;
        item.layout.visible = rect.visible;

        item.transform.anchorMin = {0.0f, 0.0f};
        item.transform.anchorMax = {0.0f, 0.0f};
        item.transform.offsetMin = {0.0f, 0.0f};
        item.transform.offsetMax = quad.bounds.size;
        item.transform.pivot = {0.0f, 0.0f};
        item.transform.scale = {1.0f, 1.0f};

        item.uvRect[0] = quad.uvRect[0];
        item.uvRect[1] = quad.uvRect[1];
        item.uvRect[2] = quad.uvRect[2];
        item.uvRect[3] = quad.uvRect[3];

        item.tintPremultipliedLinear = colorPremultiplied;
        item.textureSlice = atlasTextureSlice;
        item.materialIndex = 0;
        item.clipIndex = clipIndex;
        item.flags = 0x2; // Flag_MSDFText
        item.zOrder = zOrder;
        item.paintOrder = ++currentPaint;
        item.visible = rect.visible;

        items.push_back(item);
    }

    return items;
}

} // namespace ToolEngine::SpriteUi::Text
