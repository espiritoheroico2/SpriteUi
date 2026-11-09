#pragma once

#include "SpriteUi/Input/InputRouter.hpp"

#include <cstdint>

typedef union SDL_Event SDL_Event;
typedef struct SDL_Window SDL_Window;

namespace ToolEngine::SpriteUi::Platform {

class SDL3InputAdapter {
public:
    explicit SDL3InputAdapter(SDL_Window* window) noexcept;

    [[nodiscard]] bool IsValid() const noexcept { return windowId_ != 0; }

    // Call once for each SDL event from the application's event loop.
    // Returns true when the event belongs to this window and was processed.
    [[nodiscard]] bool ProcessEvent(const SDL_Event& event) noexcept;

    // Consume accumulated event deltas and edge transitions for one UI frame.
    [[nodiscard]] Input::UIInputSnapshot BuildSnapshot(std::uint64_t frameId) noexcept;

private:
    std::uint32_t windowId_ = 0;
    std::uint32_t pointerId_ = 0;
    Vec2 pointerPosition_{};
    Vec2 pointerDelta_{};
    Vec2 wheelDelta_{};
    bool primaryDown_ = false;
    bool primaryPressed_ = false;
    bool primaryReleased_ = false;
    bool windowFocused_ = false;
};

} // namespace ToolEngine::SpriteUi::Platform
