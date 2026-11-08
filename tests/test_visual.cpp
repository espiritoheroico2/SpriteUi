#include "SpriteUi/SpriteUi.hpp"
#include "SpriteUi/Rendering/PanelSdf.hpp"
#include "SpriteUi/Text/DebugShaper.hpp"

#include "TestHarness.hpp"
#include "TestSuites.hpp"
#include "Visual/VisualHarness.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>
#include <vector>

namespace SpriteUiTests {

namespace {

using namespace Visual;
namespace SU = ToolEngine::SpriteUi;

constexpr SU::Vec4 Orange{0.95f, 0.55f, 0.15f, 1.0f};
constexpr SU::Vec4 Green{0.25f, 0.80f, 0.40f, 1.0f};
constexpr SU::Vec4 Blue{0.20f, 0.45f, 0.95f, 1.0f};
constexpr SU::Vec4 Red{0.90f, 0.25f, 0.25f, 1.0f};
constexpr SU::Vec4 White{0.95f, 0.95f, 0.95f, 1.0f};
constexpr SU::Vec4 Cyan{0.20f, 0.85f, 0.90f, 1.0f};
constexpr SU::Vec4 Magenta{0.95f, 0.30f, 0.80f, 1.0f};
constexpr SU::Vec4 Yellow{1.0f, 0.85f, 0.20f, 1.0f};
constexpr SU::Vec4 Panel{0.13f, 0.16f, 0.22f, 1.0f};
constexpr SU::Vec4 Background{0.075f, 0.085f, 0.105f, 1.0f};

void Note(const std::string& path) {
    if (!path.empty()) std::cout << "  [snapshot] " << path << '\n';
}

// ---------------------------------------------------------------- oscilloscope window
// Visualises the fix for GetOrderedSamples: with a 64-column plot over a 300-sample history, the
// legacy call plots samples 44..107 (stale), GetLatestSamples plots 236..299 (what you want).
void TestVisualOscilloscopeWindows() {
    constexpr float Phase = 2.16f; // puts the newest sample (299) near 0.8 so its bar is tall enough to probe
    SU::Widgets::UIOscilloscopeGraph osc;
    for (int i = 0; i < 300; ++i) osc.AddSample(0.5f + 0.5f * std::sin(0.1f * static_cast<float>(i) + Phase));

    constexpr int Columns = 64;
    float oldest[Columns];
    float latest[Columns];
    osc.GetOrderedSamples(oldest);
    CHECK(osc.GetLatestSamples(latest) == Columns);

    constexpr float Baseline = 130.0f, Scale = 100.0f, BarWidth = 4.0f;
    Scene scene;
    const float leftX = 10.0f, rightX = 290.0f;
    scene.Rect({{leftX - 4, 10}, {Columns * BarWidth + 8, 130}}, Panel);
    scene.Rect({{rightX - 4, 10}, {Columns * BarWidth + 8, 130}}, Panel);
    for (int c = 0; c < Columns; ++c) {
        const float ho = oldest[c] * Scale, hl = latest[c] * Scale;
        scene.Rect({{leftX + static_cast<float>(c) * BarWidth, Baseline - ho}, {BarWidth - 1, ho}}, Orange);
        scene.Rect({{rightX + static_cast<float>(c) * BarWidth, Baseline - hl}, {BarWidth - 1, hl}}, Green);
    }
    const Image image = scene.Render(560, 150);
    Note(SaveSnapshot(image, "01_oscilloscope_oldest_vs_latest"));

    // The newest sample (299) is the last green bar and NOT part of the legacy window.
    const float newest = 0.5f + 0.5f * std::sin(0.1f * 299.0f + Phase);
    CHECK(Near(latest[Columns - 1], newest));
    const int lastX = static_cast<int>(rightX) + (Columns - 1) * 4 + 1;
    const int topY = static_cast<int>(std::lround(Baseline - newest * Scale));
    CHECK(PixelIs(image, lastX, topY + 3, Green));        // inside the bar
    CHECK(PixelIs(image, lastX, topY - 4, Panel));        // above the bar
    CHECK(!Near(oldest[Columns - 1], latest[Columns - 1], 1.0e-3f)); // different windows
    CHECK(Near(oldest[0], 0.5f + 0.5f * std::sin(0.1f * 44.0f + Phase)));
}

// ---------------------------------------------------------------- painter order vs the spec's sort key
// The specification (section 2) sorts by one 64-bit key: Canvas|Layer|Clip|Material|Sibling. Material
// sits ABOVE sibling depth, so overlapping translucent sprites are drawn in material order, not
// paint order. This scene proves the implemented rule (paint order is authoritative) is the correct one.
std::uint64_t SpecSortKey(std::uint64_t canvas, std::uint64_t layer, std::uint64_t clip,
                          std::uint64_t material, std::uint64_t sibling) {
    return (canvas << 48) | (layer << 32) | (clip << 16) | (material << 8) | sibling;
}

SU::Vec4 OverSrgb(SU::Vec4 backgroundSrgb, SU::Vec4 layerSrgb, float alpha) {
    // Premultiplied "over" in linear space, the way the pipeline blends.
    const SU::Vec4 bg = SU::Rendering::SrgbStraightToLinearPremultiplied(backgroundSrgb);
    SU::Vec4 l = SU::Rendering::SrgbStraightToLinearPremultiplied({layerSrgb.x, layerSrgb.y, layerSrgb.z, alpha});
    const float inv = 1.0f - l.w;
    return {LinearToSrgbChannel(l.x + bg.x * inv), LinearToSrgbChannel(l.y + bg.y * inv),
            LinearToSrgbChannel(l.z + bg.z * inv), 1.0f};
}

void TestVisualPaintOrderBeatsSpecSortKey() {
    struct Sprite { SU::Rect rect; SU::Vec4 color; std::uint32_t material; std::uint64_t sibling; };
    // A is BELOW B in paint order (sibling 0 < 1) but has the HIGHER material id.
    const std::vector<Sprite> sprites = {
        {{{20, 20}, {90, 90}}, {0.95f, 0.20f, 0.20f, 0.65f}, 5, 0},  // A: red, material 5, below
        {{{60, 50}, {90, 90}}, {0.20f, 0.35f, 0.95f, 0.65f}, 2, 1},  // B: blue, material 2, above
    };

    auto render = [&](bool useSpecKey) {
        std::vector<std::size_t> order = {0, 1};
        if (useSpecKey) {
            std::sort(order.begin(), order.end(), [&](std::size_t a, std::size_t b) {
                return SpecSortKey(0, 0, 0, sprites[a].material, sprites[a].sibling) <
                       SpecSortKey(0, 0, 0, sprites[b].material, sprites[b].sibling);
            });
        }
        Scene scene;
        for (std::size_t idx : order) {
            const Sprite& s = sprites[idx];
            auto& item = scene.Raw(s.rect, {s.color.x, s.color.y, s.color.z, 1.0f});
            item.tintPremultipliedLinear = SU::Rendering::SrgbStraightToLinearPremultiplied(s.color);
            item.materialIndex = s.material;
        }
        return scene.Render(170, 160);
    };

    const Image implemented = render(false); // paint order: A then B
    const Image specKey = render(true);      // spec key: material 2 (B) then material 5 (A)

    // Side-by-side for the screenshot.
    Image both(350, 160, SU::Rendering::SrgbStraightToLinearPremultiplied(Background));
    for (int y = 0; y < 160; ++y) {
        for (int x = 0; x < 170; ++x) {
            std::copy(implemented.Pixel(x, y), implemented.Pixel(x, y) + 4, both.Pixel(x, y));
            std::copy(specKey.Pixel(x, y), specKey.Pixel(x, y) + 4, both.Pixel(x + 180, y));
        }
    }
    Note(SaveSnapshot(both, "02_paint_order_vs_spec_sort_key"));

    const int ox = 90, oy = 80; // inside the overlap of A and B
    const SU::Vec4 aOnly = OverSrgb(Background, {0.95f, 0.20f, 0.20f, 1.0f}, 0.65f);
    const SU::Vec4 bOverA = {OverSrgb(aOnly, {0.20f, 0.35f, 0.95f, 1.0f}, 0.65f)};
    const SU::Vec4 aOverB = {OverSrgb(OverSrgb(Background, {0.20f, 0.35f, 0.95f, 1.0f}, 0.65f), {0.95f, 0.20f, 0.20f, 1.0f}, 0.65f)};

    CHECK(PixelIs(implemented, ox, oy, bOverA));   // B really is on top of A
    CHECK(PixelIs(specKey, ox, oy, aOverB));       // the spec key flips it: A on top of B
    CHECK(!PixelIs(implemented, ox, oy, aOverB));  // and the two results are visibly different
    CHECK(!PixelIs(specKey, ox, oy, bOverA, 8));
}

// ---------------------------------------------------------------- SDF rounded panel
void TestSdfReferenceValues() {
    using SU::Rendering::EvaluatePanelSdf;
    const SU::Vec2 half{50.0f, 20.0f};
    CHECK(Near(EvaluatePanelSdf({0, 0}, half, 10.0f), -20.0f));           // centre: distance to the nearest edge
    CHECK(Near(EvaluatePanelSdf({50, 0}, half, 10.0f), 0.0f));            // on the right edge
    CHECK(Near(EvaluatePanelSdf({60, 0}, half, 10.0f), 10.0f));           // 10 px outside the right edge
    CHECK(Near(EvaluatePanelSdf({50, 20}, half, 10.0f), std::sqrt(200.0f) - 10.0f)); // the cut corner is OUTSIDE
    CHECK(Near(EvaluatePanelSdf({50, 20}, half, 0.0f), 0.0f));            // square corner is ON the edge
    // A radius larger than the half extent is clamped (pill), instead of degenerating.
    CHECK(Near(EvaluatePanelSdf({0, 0}, half, 999.0f), EvaluatePanelSdf({0, 0}, half, 20.0f)));
    CHECK(Near(EvaluatePanelSdf({0, 0}, half, -5.0f), EvaluatePanelSdf({0, 0}, half, 0.0f)));
    CHECK(std::isfinite(EvaluatePanelSdf({1, 1}, half, std::nanf(""))));
    CHECK(Near(SU::Rendering::PanelCoverage(-3.0f, 1.0f), 1.0f));
    CHECK(Near(SU::Rendering::PanelCoverage(0.0f, 1.0f), 0.5f));
    CHECK(Near(SU::Rendering::PanelCoverage(3.0f, 1.0f), 0.0f));
}

void TestVisualSdfPanels() {
    Scene scene;
    const float radii[] = {0.0f, 8.0f, 24.0f, 1000.0f};
    for (int i = 0; i < 4; ++i) {
        const SU::Rect r{{20.0f + 140.0f * static_cast<float>(i), 20.0f}, {120.0f, 70.0f}};
        scene.Rect(r, Blue, SU::Rendering::WhiteTextureSlice, SU::Rendering::FlagRoundedPanel, radii[i]);
    }
    scene.Rect({{20, 110}, {540, 40}}, Green, 0, SU::Rendering::FlagRoundedPanel, 20.0f); // pill
    const Image image = scene.Render(580, 170);
    Note(SaveSnapshot(image, "03_sdf_rounded_panels_dpi1"));
    const Image retina = scene.Render(1160, 340, 2.0f);
    Note(SaveSnapshot(retina, "04_sdf_rounded_panels_dpi2"));

    // radius 0: the corner pixel is filled; radius 24: the corner is cut away; centre always filled.
    CHECK(PixelIs(image, 21, 21, Blue));
    CHECK(PixelIs(image, 20 + 140 * 2 + 1, 21, Background));
    CHECK(PixelIs(image, 20 + 140 * 2 + 60, 55, Blue));
    // radius 1000 clamps to 35 (half of the shorter side): a pill-ish shape, corner empty, middle full.
    CHECK(PixelIs(image, 20 + 140 * 3 + 1, 21, Background));
    CHECK(PixelIs(image, 20 + 140 * 3 + 60, 55, Blue));
    // pill: the left end is a semicircle.
    CHECK(PixelIs(image, 21, 111, Background));
    CHECK(PixelIs(image, 40, 130, Green));
    // DPI 2 renders the same shape twice as large, still with cut corners.
    CHECK(PixelIs(retina, 2 * (20 + 140 * 2) + 2, 42, Background));
    CHECK(PixelIs(retina, 2 * (20 + 140 * 2 + 60), 110, Blue));
}

// ---------------------------------------------------------------- clip hierarchy
void TestVisualClipHierarchy() {
    using namespace SU::Rendering;
    std::vector<ClipNode> nodes(3);
    nodes[0].rect = {40, 30, 240, 170};                        // outer window
    nodes[1].parentIndex = 0; nodes[1].rect = {120, 60, 360, 200}; // scroll area, sticks out of the window
    nodes[2].parentIndex = 1; nodes[2].enabled = false;        // disabled node inherits its parent
    const ClipHierarchyResult clips = BuildClipHierarchy(nodes);
    CHECK(clips.valid);
    CHECK(clips.effectiveRects.size() == 2);
    CHECK(Near(clips.effectiveRects[1].minX, 120.0f) && Near(clips.effectiveRects[1].maxX, 240.0f));
    CHECK(clips.nodeClipIndices[2] == clips.nodeClipIndices[1]);

    Scene scene;
    for (const auto& rect : clips.effectiveRects) scene.AddClip(rect);
    scene.Outline({{40, 30}, {200, 140}}, 1.0f, Cyan);
    scene.Rect({{0, 0}, {400, 230}}, SU::Vec4{0.30f, 0.30f, 0.38f, 1.0f}, 0, 0, 0, clips.nodeClipIndices[0]);
    scene.Rect({{0, 0}, {400, 230}}, Orange, 0, 0, 0, clips.nodeClipIndices[1]);
    const Image image = scene.Render(400, 230);
    Note(SaveSnapshot(image, "05_clip_hierarchy"));

    CHECK(PixelIs(image, 60, 50, {0.30f, 0.30f, 0.38f, 1.0f}));  // inside the window, outside the scroll area
    CHECK(PixelIs(image, 200, 100, Orange));                      // inside the intersection
    CHECK(PixelIs(image, 300, 100, Background));                  // the scroll area sticks out: clipped by the window
    CHECK(PixelIs(image, 200, 190, Background));                  // below the window: clipped
    CHECK(PixelIs(image, 20, 20, Background));                    // outside everything
}

} // namespace

void RunVisualTests() {
    std::cout << "[visual] screenshots -> " << Visual::SnapshotDir().string() << '\n';
    TestVisualOscilloscopeWindows();
    TestVisualPaintOrderBeatsSpecSortKey();
    TestSdfReferenceValues();
    TestVisualSdfPanels();
    TestVisualClipHierarchy();
}

} // namespace SpriteUiTests
