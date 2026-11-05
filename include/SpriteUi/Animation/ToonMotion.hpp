#pragma once

#include "SpriteUi/Core/Handle.hpp"
#include "SpriteUi/Core/Math.hpp"

#include <cstdint>
#include <vector>

namespace ToolEngine::SpriteUi::Animation {

enum class TweenProperty : std::uint8_t { Position, Scale, Rotation, Opacity, VisualOffset };
enum class TweenConflict : std::uint8_t { Override, Queue, Ignore };
enum class Easing : std::uint8_t { Linear, EaseOutQuad, EaseInOutCubic, SmoothStep };
enum class TweenState : std::uint8_t { Playing, Paused };

struct TweenTrack {
    std::uint64_t id = 0;
    UIHandle target{};
    TweenProperty property = TweenProperty::Position;
    Vec4 from{};
    Vec4 to{};
    float duration = 0.25f;
    float elapsed = 0.0f;
    Easing easing = Easing::EaseOutQuad;
    TweenState state = TweenState::Playing;
    bool queued = false;
};

struct TweenSample {
    std::uint64_t trackId = 0;
    UIHandle target{};
    TweenProperty property = TweenProperty::Position;
    Vec4 value{};
};

struct TweenCompleted {
    std::uint64_t trackId = 0;
    UIHandle target{};
    TweenProperty property = TweenProperty::Position;
};

struct TweenUpdate {
    std::vector<TweenSample> samples;
    std::vector<TweenCompleted> completed;
};

// Aplica opacidade a uma cor ja pre-multiplicada (V4.2 sec. 5): RGB e A escalam juntos,
// preservando as proporcoes RGB/A sem degradar cores emissivas.
[[nodiscard]] inline Vec4 ApplyOpacityPremultiplied(Vec4 premultipliedTint, float opacity) noexcept {
    if (!IsFinite(premultipliedTint) || !IsFinite(opacity)) return {};
    return premultipliedTint * Clamp01(opacity);
}

[[nodiscard]] float EvaluateEasing(Easing easing, float t) noexcept;
[[nodiscard]] Vec2 EvaluateShake(
    float elapsed, float duration, Vec2 amplitude, float frequency = 38.0f) noexcept;

class ToonMotionSystem {
public:
    [[nodiscard]] bool Add(TweenTrack track, TweenConflict conflict = TweenConflict::Override);
    [[nodiscard]] TweenUpdate Update(float deltaTime);
    [[nodiscard]] bool SetPaused(std::uint64_t trackId, bool paused) noexcept;
    void CancelTarget(UIHandle target) noexcept;
    void Clear() noexcept { tracks_.clear(); }
    [[nodiscard]] std::size_t Size() const noexcept { return tracks_.size(); }

private:
    [[nodiscard]] bool HasTrack(UIHandle target, TweenProperty property) const noexcept;
    std::vector<TweenTrack> tracks_;
    std::uint64_t nextId_ = 1;
};

} // namespace ToolEngine::SpriteUi::Animation
