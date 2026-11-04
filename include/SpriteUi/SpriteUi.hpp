#pragma once

// Public umbrella header for the backend-agnostic SpriteUI MVP.
#include "SpriteUi/Core/Handle.hpp"
#include "SpriteUi/Core/Math.hpp"
#include "SpriteUi/Core/DirtyFlags.hpp"
#include "SpriteUi/Core/Transform2D.hpp"
#include "SpriteUi/Input/InputRouter.hpp"
#include "SpriteUi/Input/SpatialNavigation.hpp"
#include "SpriteUi/Widgets/Widgets.hpp"
#include "SpriteUi/Animation/ToonMotion.hpp"
#include "SpriteUi/Rendering/SpriteInstanceGPU.hpp"
#include "SpriteUi/Rendering/CanvasConfig.hpp"
#include "SpriteUi/Text/GlyphInstance.hpp"
#include "SpriteUi/Rendering/DrawList.hpp"
#include "SpriteUi/Rendering/IRenderBackend.hpp"

#include "SpriteUi/Layout/Layout.hpp"
#include "SpriteUi/Layout/LayoutTree.hpp"
