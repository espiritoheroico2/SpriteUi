#pragma once

#include "SpriteUi/Core/Handle.hpp"
#include "SpriteUi/Core/Math.hpp"

#include <cstdint>
#include <span>
#include <vector>

namespace ToolEngine::SpriteUi::Input {

enum class NavDirection : std::uint8_t { Up, Down, Left, Right };
enum class UISemanticAction : std::uint8_t { Activate, Cancel };

struct UIFocusCandidate {
    UIHandle handle{};
    Rect bounds{};
    bool focusable = true;
    bool enabled = true;
    bool visible = true;
    std::int32_t navigationOrder = 0;
};

// Selects the nearest candidate in a 90-degree directional cone (±45 degrees).
// The caller sends Activate/Cancel to the focused widget as semantic events.
[[nodiscard]] inline UIHandle FindSpatialNeighbor(
    UIHandle current,
    NavDirection direction,
    std::span<const UIFocusCandidate> candidates) noexcept
{
    const UIFocusCandidate* origin = nullptr;
    for (const auto& candidate : candidates) {
        if (candidate.handle == current && candidate.visible && candidate.enabled && candidate.focusable) {
            origin = &candidate;
            break;
        }
    }
    if (!origin || !IsFinite(origin->bounds.position) || !IsFinite(origin->bounds.size) ||
        origin->bounds.size.x <= 0.0f || origin->bounds.size.y <= 0.0f) {
        return InvalidUIHandle;
    }

    Vec2 axis{};
    switch (direction) {
    case NavDirection::Up:    axis = {0.0f, -1.0f}; break;
    case NavDirection::Down:  axis = {0.0f, 1.0f}; break;
    case NavDirection::Left:  axis = {-1.0f, 0.0f}; break;
    case NavDirection::Right: axis = {1.0f, 0.0f}; break;
    }

    const Vec2 originCenter = origin->bounds.position + origin->bounds.size * 0.5f;
    UIHandle best = InvalidUIHandle;
    float bestScore = 0.0f;

    for (const auto& candidate : candidates) {
        if (candidate.handle == current || !candidate.handle.IsValid() ||
            !candidate.visible || !candidate.enabled || !candidate.focusable ||
            !IsFinite(candidate.bounds.position) || !IsFinite(candidate.bounds.size) ||
            candidate.bounds.size.x <= 0.0f || candidate.bounds.size.y <= 0.0f) {
            continue;
        }

        const Vec2 center = candidate.bounds.position + candidate.bounds.size * 0.5f;
        const Vec2 delta = center - originCenter;
        const float forward = Dot(delta, axis);
        const float side = std::abs(delta.x * axis.y - delta.y * axis.x);
        if (forward <= 0.0f || side > forward) continue;

        const float score = forward + side * 2.0f;
        if (!best.IsValid() || score < bestScore ||
            (score == bestScore && candidate.handle.Pack() < best.Pack())) {
            best = candidate.handle;
            bestScore = score;
        }
    }
    return best;
}


enum class UINavigationCommand : std::uint8_t {
    None, Up, Down, Left, Right, Activate, Cancel
};

enum class UISemanticEventType : std::uint8_t {
    FocusChanged, Activate, Cancel
};

struct UISemanticEvent {
    UISemanticEventType type = UISemanticEventType::FocusChanged;
    UIHandle target{};
    UIHandle previousTarget{};
};

// Stateful semantic navigation. Platform adapters (SDL, GLFW, XInput, etc.)
// translate key/gamepad edges into one command and feed it here.
class UINavigationRouter {
public:
    [[nodiscard]] UIHandle Focused() const noexcept { return focused_; }

    [[nodiscard]] std::vector<UISemanticEvent> Process(
        UINavigationCommand command,
        std::span<const UIFocusCandidate> candidates)
    {
        std::vector<UISemanticEvent> events;
        const UIHandle previous = focused_;
        const UIFocusCandidate* focusedCandidate = FindCandidate(focused_, candidates);

        if (!focusedCandidate) {
            focused_ = FindFirst(candidates);
            if (focused_ != previous) {
                events.push_back({UISemanticEventType::FocusChanged, focused_, previous});
            }
        }

        UIHandle next = focused_;
        switch (command) {
        case UINavigationCommand::Up:
            next = FindSpatialNeighbor(focused_, NavDirection::Up, candidates);
            break;
        case UINavigationCommand::Down:
            next = FindSpatialNeighbor(focused_, NavDirection::Down, candidates);
            break;
        case UINavigationCommand::Left:
            next = FindSpatialNeighbor(focused_, NavDirection::Left, candidates);
            break;
        case UINavigationCommand::Right:
            next = FindSpatialNeighbor(focused_, NavDirection::Right, candidates);
            break;
        case UINavigationCommand::None:
        case UINavigationCommand::Activate:
        case UINavigationCommand::Cancel:
            break;
        }

        if (next.IsValid() && next != focused_) {
            const UIHandle old = focused_;
            focused_ = next;
            events.push_back({UISemanticEventType::FocusChanged, focused_, old});
        }

        if (command == UINavigationCommand::Activate && focused_.IsValid()) {
            events.push_back({UISemanticEventType::Activate, focused_, focused_});
        } else if (command == UINavigationCommand::Cancel && focused_.IsValid()) {
            events.push_back({UISemanticEventType::Cancel, focused_, focused_});
        }

        return events;
    }

    void SetFocused(UIHandle handle) noexcept { focused_ = handle; }
    void Reset() noexcept { focused_ = InvalidUIHandle; }

private:
    [[nodiscard]] static bool IsUsable(const UIFocusCandidate& candidate) noexcept {
        return candidate.handle.IsValid() && candidate.focusable && candidate.enabled &&
               candidate.visible && IsFinite(candidate.bounds.position) &&
               IsFinite(candidate.bounds.size) && candidate.bounds.size.x > 0.0f &&
               candidate.bounds.size.y > 0.0f;
    }

    [[nodiscard]] static const UIFocusCandidate* FindCandidate(
        UIHandle handle, std::span<const UIFocusCandidate> candidates) noexcept
    {
        if (!handle.IsValid()) return nullptr;
        for (const auto& candidate : candidates) {
            if (candidate.handle == handle && IsUsable(candidate)) return &candidate;
        }
        return nullptr;
    }

    [[nodiscard]] static UIHandle FindFirst(
        std::span<const UIFocusCandidate> candidates) noexcept
    {
        const UIFocusCandidate* best = nullptr;
        for (const auto& candidate : candidates) {
            if (!IsUsable(candidate)) continue;
            if (!best || candidate.navigationOrder < best->navigationOrder ||
                (candidate.navigationOrder == best->navigationOrder &&
                 candidate.handle.Pack() < best->handle.Pack())) {
                best = &candidate;
            }
        }
        return best ? best->handle : InvalidUIHandle;
    }

    UIHandle focused_ = InvalidUIHandle;
};

} // namespace ToolEngine::SpriteUi::Input
