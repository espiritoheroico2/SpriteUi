#include "SpriteUi/Animation/ToonMotion.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace ToolEngine::SpriteUi::Animation {

float EvaluateEasing(Easing easing, float t) noexcept {
    t = Clamp01(t);
    switch (easing) {
    case Easing::Linear:
        return t;
    case Easing::EaseOutQuad:
        return t * (2.0f - t);
    case Easing::EaseInOutCubic:
        return t < 0.5f ? 4.0f * t * t * t
                        : 1.0f - std::pow(-2.0f * t + 2.0f, 3.0f) * 0.5f;
    case Easing::SmoothStep:
        return t * t * (3.0f - 2.0f * t);
    }
    return t;
}

Vec2 EvaluateShake(float elapsed, float duration, Vec2 amplitude, float frequency) noexcept {
    if (!IsFinite(elapsed) || !IsFinite(duration) || !IsFinite(amplitude) ||
        !IsFinite(frequency) || duration <= 0.0f || frequency <= 0.0f ||
        elapsed >= duration) {
        return {};
    }

    const float t = Clamp01(elapsed / duration);
    const float envelope = (1.0f - t) * (1.0f - t);
    constexpr float phase = 1.61803398875f;
    return {
        amplitude.x * envelope * std::sin(elapsed * frequency),
        amplitude.y * envelope * std::sin(elapsed * frequency * phase)
    };
}

bool ToonMotionSystem::HasTrack(UIHandle target, TweenProperty property) const noexcept {
    return std::any_of(tracks_.begin(), tracks_.end(), [&](const TweenTrack& track) {
        return track.target == target && track.property == property;
    });
}

bool ToonMotionSystem::Add(TweenTrack track, TweenConflict conflict) {
    if (!track.target.IsValid() || !IsFinite(track.from) || !IsFinite(track.to) ||
        !IsFinite(track.duration) || track.duration < 0.0f ||
        static_cast<std::uint8_t>(track.property) >
            static_cast<std::uint8_t>(TweenProperty::VisualOffset) ||
        static_cast<std::uint8_t>(track.easing) >
            static_cast<std::uint8_t>(Easing::SmoothStep)) {
        return false;
    }
    if (track.id == std::numeric_limits<std::uint64_t>::max()) return false;
    if (track.id != 0 && std::any_of(tracks_.begin(), tracks_.end(),
        [&](const TweenTrack& existing) { return existing.id == track.id; })) {
        return false;
    }

    const bool exists = HasTrack(track.target, track.property);
    if (exists && conflict == TweenConflict::Ignore) return false;
    if (exists && conflict == TweenConflict::Override) {
        tracks_.erase(std::remove_if(tracks_.begin(), tracks_.end(),
            [&](const TweenTrack& existing) {
                return existing.target == track.target &&
                       existing.property == track.property;
            }), tracks_.end());
    }

    if (track.id == 0) {
        while (nextId_ != 0 && nextId_ != std::numeric_limits<std::uint64_t>::max() &&
               std::any_of(tracks_.begin(), tracks_.end(),
                   [&](const TweenTrack& existing) { return existing.id == nextId_; })) {
            ++nextId_;
        }
        if (nextId_ == 0 || nextId_ == std::numeric_limits<std::uint64_t>::max()) return false;
        track.id = nextId_++;
    } else if (track.id >= nextId_) {
        nextId_ = track.id + 1;
    }
    track.elapsed = 0.0f;
    track.queued = exists && conflict == TweenConflict::Queue;
    track.state = TweenState::Playing;
    tracks_.push_back(track);
    return true;
}

TweenUpdate ToonMotionSystem::Update(float deltaTime) {
    TweenUpdate result;
    if (!IsFinite(deltaTime) || deltaTime < 0.0f) return result;

    // Queued tracks become active when no earlier track for the same property remains.
    for (std::size_t i = 0; i < tracks_.size(); ++i) {
        TweenTrack& track = tracks_[i];
        if (!track.queued || track.state == TweenState::Paused) continue;

        bool earlierActive = false;
        for (std::size_t j = 0; j < i; ++j) {
            if (tracks_[j].target == track.target &&
                tracks_[j].property == track.property) {
                earlierActive = true;
                break;
            }
        }
        if (!earlierActive) track.queued = false;
    }

    for (TweenTrack& track : tracks_) {
        if (track.queued || track.state == TweenState::Paused) continue;

        track.elapsed += deltaTime;
        const float progress = track.duration <= 0.0f
            ? 1.0f
            : Clamp01(track.elapsed / track.duration);
        const float eased = EvaluateEasing(track.easing, progress);
        result.samples.push_back({
            track.id, track.target, track.property, Lerp(track.from, track.to, eased)
        });

        if (progress >= 1.0f) {
            result.completed.push_back({track.id, track.target, track.property});
        }
    }

    if (!result.completed.empty()) {
        tracks_.erase(std::remove_if(tracks_.begin(), tracks_.end(),
            [&](const TweenTrack& track) {
                return std::any_of(result.completed.begin(), result.completed.end(),
                    [&](const TweenCompleted& completed) {
                        return completed.trackId == track.id;
                    });
            }), tracks_.end());
    }

    return result;
}

bool ToonMotionSystem::SetPaused(std::uint64_t trackId, bool paused) noexcept {
    for (TweenTrack& track : tracks_) {
        if (track.id == trackId) {
            track.state = paused ? TweenState::Paused : TweenState::Playing;
            return true;
        }
    }
    return false;
}

void ToonMotionSystem::CancelTarget(UIHandle target) noexcept {
    tracks_.erase(std::remove_if(tracks_.begin(), tracks_.end(),
        [&](const TweenTrack& track) { return track.target == target; }), tracks_.end());
}

} // namespace ToolEngine::SpriteUi::Animation
