#include "SpriteUi/Platform/SDL3InputAdapter.hpp"

#include <SDL3/SDL.h>

namespace ToolEngine::SpriteUi::Platform {

SDL3InputAdapter::SDL3InputAdapter(SDL_Window* window) noexcept {
    if (!window) return;
    windowId_ = SDL_GetWindowID(window);
    windowFocused_ = windowId_ != 0 &&
                     (SDL_GetWindowFlags(window) & SDL_WINDOW_INPUT_FOCUS) != 0;
}

bool SDL3InputAdapter::ProcessEvent(const SDL_Event& event) noexcept {
    switch (event.type) {
    case SDL_EVENT_MOUSE_MOTION:
        if (event.motion.windowID != windowId_ || windowId_ == 0) return false;
        pointerId_ = event.motion.which;
        pointerPosition_ = {event.motion.x, event.motion.y};
        pointerDelta_.x += event.motion.xrel;
        pointerDelta_.y += event.motion.yrel;
        return true;

    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    case SDL_EVENT_MOUSE_BUTTON_UP:
        if (event.button.windowID != windowId_ || windowId_ == 0 ||
            event.button.button != SDL_BUTTON_LEFT) {
            return false;
        }
        pointerId_ = event.button.which;
        pointerPosition_ = {event.button.x, event.button.y};
        if (event.button.down && !primaryDown_) primaryPressed_ = true;
        if (!event.button.down && primaryDown_) primaryReleased_ = true;
        primaryDown_ = event.button.down;
        return true;

    case SDL_EVENT_MOUSE_WHEEL:
        if (event.wheel.windowID != windowId_ || windowId_ == 0) return false;
        pointerId_ = event.wheel.which;
        pointerPosition_ = {event.wheel.mouse_x, event.wheel.mouse_y};
        if (event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED) {
            wheelDelta_.x -= event.wheel.x;
            wheelDelta_.y -= event.wheel.y;
        } else {
            wheelDelta_.x += event.wheel.x;
            wheelDelta_.y += event.wheel.y;
        }
        return true;

    case SDL_EVENT_WINDOW_FOCUS_GAINED:
        if (event.window.windowID != windowId_ || windowId_ == 0) return false;
        windowFocused_ = true;
        return true;

    case SDL_EVENT_WINDOW_FOCUS_LOST:
        if (event.window.windowID != windowId_ || windowId_ == 0) return false;
        windowFocused_ = false;
        primaryDown_ = false;
        primaryPressed_ = false;
        primaryReleased_ = false;
        return true;

    default:
        return false;
    }
}

Input::UIInputSnapshot SDL3InputAdapter::BuildSnapshot(std::uint64_t frameId) noexcept {
    Input::UIInputSnapshot snapshot;
    snapshot.pointerPositionLogical = pointerPosition_;
    snapshot.pointerDeltaLogical = pointerDelta_;
    snapshot.wheelDelta = wheelDelta_;
    snapshot.pointerId = pointerId_;
    snapshot.primaryDown = primaryDown_ && windowFocused_;
    snapshot.primaryPressed = primaryPressed_ && windowFocused_;
    snapshot.primaryReleased = primaryReleased_ && windowFocused_;
    snapshot.windowFocused = windowFocused_;
    snapshot.frameId = frameId;

    pointerDelta_ = {};
    wheelDelta_ = {};
    primaryPressed_ = false;
    primaryReleased_ = false;
    return snapshot;
}

} // namespace ToolEngine::SpriteUi::Platform
