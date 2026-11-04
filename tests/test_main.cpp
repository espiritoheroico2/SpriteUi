#include "SpriteUi/SpriteUi.hpp"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

using namespace ToolEngine::SpriteUi;

namespace {

int failures = 0;
int testsRun = 0;

void Check(bool condition, const char* expression, const char* testName) {
    ++testsRun;
    if (!condition) {
        ++failures;
        std::cerr << "[FAIL] " << testName << ": " << expression << '\n';
    }
}

#define CHECK(expr) Check((expr), #expr, __func__)

bool Near(float a, float b, float epsilon = 1.0e-4f) {
    return std::abs(a - b) <= epsilon;
}

void TestAnchorsAndTransforms() {
    UIRectTransform transform;
    transform.anchorMin = {0.0f, 0.0f};
    transform.anchorMax = {1.0f, 1.0f};
    transform.offsetMin = {10.0f, 20.0f};
    transform.offsetMax = {-10.0f, -20.0f};

    const auto rect = ResolveAnchoredRect({{0, 0}, {800, 600}}, transform);
    CHECK(rect.has_value());
    CHECK(Near(rect->position.x, 10.0f));
    CHECK(Near(rect->size.x, 780.0f));
    CHECK(Near(rect->size.y, 560.0f));

    UIRectTransform invalidAnchors;
    invalidAnchors.anchorMin = {0.8f, 0.0f};
    invalidAnchors.anchorMax = {0.2f, 1.0f};
    CHECK(!ResolveAnchoredRect({{0, 0}, {800, 600}}, invalidAnchors).has_value());

    UILayoutRect layout{rect->position, rect->size, true, false};
    transform.rotationRadians = 0.4f;
    transform.pivot = {0.5f, 0.5f};
    const auto affine = BuildUnitQuadTransform(layout, transform);
    CHECK(affine.has_value());

    const Vec2 pivotScreen = affine->TransformPoint({0.5f, 0.5f});
    CHECK(Near(pivotScreen.x, rect->position.x + rect->size.x * 0.5f));
    CHECK(Near(pivotScreen.y, rect->position.y + rect->size.y * 0.5f));

    const auto inverse = affine->Inverse();
    CHECK(inverse.has_value());
    const Vec2 roundTrip = inverse->TransformPoint(affine->TransformPoint({0.25f, 0.75f}));
    CHECK(Near(roundTrip.x, 0.25f));
    CHECK(Near(roundTrip.y, 0.75f));
}

void TestInputRouterClickAndCapture() {
    using namespace ToolEngine::SpriteUi::Input;

    UIEventRouter router;
    HitTarget target;
    target.handle = {10, 1};
    target.localToScreen = {1, 0, 100, 0, 1, 100};
    target.localSize = {100, 50};
    target.paintOrder = 1;
    target.focusable = true;

    UIInputSnapshot down;
    down.frameId = 1;
    down.pointerPositionLogical = {120, 120};
    down.primaryDown = true;
    down.primaryPressed = true;
    auto downEvents = router.Process(down, std::span<const HitTarget>(&target, 1));
    CHECK(router.GetState().capturedEntity == target.handle);
    CHECK(router.GetState().focusedEntity == target.handle);
    CHECK(!downEvents.empty());

    UIInputSnapshot up;
    up.frameId = 2;
    up.pointerPositionLogical = {130, 120};
    up.primaryReleased = true;
    auto upEvents = router.Process(up, std::span<const HitTarget>(&target, 1));
    bool clicked = false;
    for (const auto& event : upEvents) clicked |= event.type == UIEventType::Click;
    CHECK(clicked);
    CHECK(!router.GetState().capturedEntity.IsValid());
}

void TestInputRouterCancelAndOcclusion() {
    using namespace ToolEngine::SpriteUi::Input;

    UIEventRouter router;
    HitTarget lower;
    lower.handle = {1, 1};
    lower.localToScreen.tx = 0;
    lower.localToScreen.ty = 0;
    lower.localSize = {100, 100};
    lower.paintOrder = 1;

    HitTarget upper = lower;
    upper.handle = {2, 1};
    upper.paintOrder = 2;
    upper.blocksInput = true;

    const HitTarget targets[] = {lower, upper};
    UIInputSnapshot down;
    down.frameId = 10;
    down.pointerPositionLogical = {20, 20};
    down.primaryPressed = true;
    down.primaryDown = true;
    const auto downEvents = router.Process(down, targets);
    CHECK(!downEvents.empty());
    CHECK(router.GetState().capturedEntity == upper.handle);
    CHECK(router.GetState().consumedThisFrame);

    UIInputSnapshot lost;
    lost.frameId = 11;
    lost.windowFocused = false;
    const auto events = router.Process(lost, targets);
    bool cancelled = false;
    for (const auto& event : events) cancelled |= event.type == UIEventType::Cancel;
    CHECK(cancelled);
    CHECK(!router.GetState().capturedEntity.IsValid());
}

void TestHoverMoveEvent() {
    using namespace ToolEngine::SpriteUi::Input;
    UIEventRouter router;
    HitTarget target;
    target.handle = {55, 1};
    target.localSize = {80, 40};
    target.paintOrder = 1;

    UIInputSnapshot input;
    input.frameId = 3;
    input.pointerPositionLogical = {10, 10};
    input.pointerDeltaLogical = {2, 1};
    const auto events = router.Process(input, std::span<const HitTarget>(&target, 1));
    bool moved = false;
    for (const auto& event : events) moved |= event.type == UIEventType::PointerMove;
    CHECK(moved);
}

void TestSpatialNavigationAndDirtyFlags() {
    using namespace ToolEngine::SpriteUi::Input;
    const UIFocusCandidate candidates[] = {
        {{1, 1}, {{0, 0}, {40, 40}}, true, true, true},
        {{2, 1}, {{60, 0}, {40, 40}}, true, true, true},
        {{3, 1}, {{0, 60}, {40, 40}}, true, true, true},
        {{4, 1}, {{-70, 0}, {40, 40}}, true, true, true}
    };
    CHECK(FindSpatialNeighbor({1, 1}, NavDirection::Right, candidates) == (UIHandle{2, 1}));
    CHECK(FindSpatialNeighbor({1, 1}, NavDirection::Down, candidates) == (UIHandle{3, 1}));
    CHECK(FindSpatialNeighbor({1, 1}, NavDirection::Left, candidates) == (UIHandle{4, 1}));

    UIDirtyState dirty;
    CHECK(dirty.Has(UIDirtyBits::Measure));
    dirty.Clear(UIDirtyBits::Measure);
    CHECK(!dirty.Has(UIDirtyBits::Measure));
    dirty.Mark(UIDirtyBits::TextShape | UIDirtyBits::DrawData);
    CHECK(dirty.Has(UIDirtyBits::TextShape));
    CHECK((dirty.Consume() & static_cast<std::uint32_t>(UIDirtyBits::DrawData)) != 0);
    CHECK(dirty.bits == 0);
}


void TestInvalidInputCancelsCapture() {
    using namespace ToolEngine::SpriteUi::Input;
    UIEventRouter router;
    HitTarget target;
    target.handle = {777, 1};
    target.localSize = {100.0f, 50.0f};

    UIInputSnapshot down;
    down.frameId = 1;
    down.pointerPositionLogical = {10.0f, 10.0f};
    down.primaryPressed = true;
    down.primaryDown = true;
    (void)router.Process(down, std::span<const HitTarget>(&target, 1));
    CHECK(router.GetState().capturedEntity == target.handle);

    UIInputSnapshot invalid;
    invalid.frameId = 2;
    invalid.pointerPositionLogical = {10.0f, 10.0f};
    invalid.pointerDeltaLogical.x = std::numeric_limits<float>::quiet_NaN();
    const auto events = router.Process(invalid, std::span<const HitTarget>(&target, 1));
    bool cancelled = false;
    for (const auto& event : events) cancelled |= event.type == UIEventType::Cancel;
    CHECK(cancelled);
    CHECK(!router.GetState().capturedEntity.IsValid());
}

void TestHitTestTransformAndMaxFrameId() {
    using namespace ToolEngine::SpriteUi::Input;

    UILayoutRect layout{{100.0f, 100.0f}, {200.0f, 100.0f}, true, false};
    UIRectTransform transform;
    transform.pivot = {0.5f, 0.5f};
    transform.scale = {1.5f, 0.75f};
    transform.rotationRadians = 1.57079632679f;
    const auto hitTransform = BuildHitTestTransform(layout, transform);
    CHECK(hitTransform.has_value());
    if (!hitTransform) return;

    const Vec2 center = hitTransform->TransformPoint({100.0f, 50.0f});
    CHECK(Near(center.x, 200.0f));
    CHECK(Near(center.y, 150.0f));

    HitTarget target;
    target.handle = {901, 1};
    target.localToScreen = *hitTransform;
    target.localSize = layout.resolvedSize;
    target.focusable = true;

    UIEventRouter router;
    UIInputSnapshot input;
    input.frameId = UINT64_MAX; // Must be processed even on the first call.
    input.pointerPositionLogical = center;
    input.primaryPressed = true;
    input.primaryDown = true;
    const auto events = router.Process(input, std::span<const HitTarget>(&target, 1));
    CHECK(router.GetState().capturedEntity == target.handle);
    bool down = false;
    for (const auto& event : events) {
        down |= event.type == UIEventType::PointerDown &&
                Near(event.localPosition.x, 100.0f) && Near(event.localPosition.y, 50.0f);
    }
    CHECK(down);

    // Malformed dimensions must never create a hit target.
    HitTarget malformed = target;
    malformed.handle = {902, 1};
    malformed.localSize.x = std::numeric_limits<float>::quiet_NaN();
    UIEventRouter invalidRouter;
    UIInputSnapshot invalidInput;
    invalidInput.frameId = 1;
    invalidInput.pointerPositionLogical = center;
    invalidInput.primaryPressed = true;
    const auto invalidEvents = invalidRouter.Process(
        invalidInput, std::span<const HitTarget>(&malformed, 1));
    CHECK(!invalidRouter.GetState().capturedEntity.IsValid());
    CHECK(invalidEvents.empty());
}


void TestEventBubblingAndHoverConsumption() {
    using namespace ToolEngine::SpriteUi::Input;
    HitTarget parent;
    parent.handle = {100, 1};
    parent.localSize = {200, 100};
    parent.blocksInput = true;
    parent.paintOrder = 1;

    HitTarget child;
    child.handle = {101, 1};
    child.parent = parent.handle;
    child.localSize = {50, 30};
    child.localToScreen.tx = 20.0f;
    child.localToScreen.ty = 20.0f;
    child.blocksInput = false;
    child.paintOrder = 2;

    const HitTarget targets[] = {parent, child};
    UIEventRouter router;
    UIInputSnapshot hover;
    hover.frameId = 20;
    hover.pointerPositionLogical = {25.0f, 25.0f};
    const auto hoverEvents = router.Process(hover, targets);
    CHECK(!hoverEvents.empty());
    CHECK(router.GetState().consumedThisFrame); // Parent blocks world input.

    UIInputSnapshot down;
    down.frameId = 21;
    down.pointerPositionLogical = {25.0f, 25.0f};
    down.primaryPressed = true;
    down.primaryDown = true;
    const auto events = router.Process(down, targets);
    bool childDown = false;
    bool parentDown = false;
    for (const auto& event : events) {
        if (event.type != UIEventType::PointerDown) continue;
        childDown |= event.target == child.handle;
        parentDown |= event.target == parent.handle;
    }
    CHECK(childDown);
    CHECK(parentDown);
    CHECK(router.GetState().consumedThisFrame);
}

void TestSemanticNavigationRouter() {
    using namespace ToolEngine::SpriteUi::Input;
    const UIFocusCandidate candidates[] = {
        {{11, 1}, {{0, 0}, {40, 40}}, true, true, true, 0},
        {{12, 1}, {{60, 0}, {40, 40}}, true, true, true, 1},
        {{13, 1}, {{0, 60}, {40, 40}}, true, true, true, 2}
    };

    UINavigationRouter router;
    const auto moved = router.Process(UINavigationCommand::Right, candidates);
    CHECK(router.Focused() == (UIHandle{12, 1}));
    CHECK(moved.size() == 2); // Initial focus, then directional focus change.
    CHECK(moved.back().type == UISemanticEventType::FocusChanged);

    const auto activated = router.Process(UINavigationCommand::Activate, candidates);
    CHECK(activated.size() == 1);
    CHECK(activated.front().type == UISemanticEventType::Activate);
    CHECK(activated.front().target == (UIHandle{12, 1}));

    const auto cancelled = router.Process(UINavigationCommand::Cancel, candidates);
    CHECK(cancelled.size() == 1);
    CHECK(cancelled.front().type == UISemanticEventType::Cancel);

    const UIFocusCandidate disabled[] = {
        {{12, 1}, {{60, 0}, {40, 40}}, true, false, true, 1}
    };
    const auto lost = router.Process(UINavigationCommand::None, disabled);
    CHECK(!router.Focused().IsValid());
    CHECK(lost.size() == 1);
    CHECK(lost.front().type == UISemanticEventType::FocusChanged);
}

void TestCanvasConfig() {
    using namespace ToolEngine::SpriteUi::Rendering;
    UICanvasRenderConfig config;
    CHECK(config.IsValid());
    config.shading = UIShadingMode::ClusterLit;
    CHECK(!config.IsValid());
    config.space = UICanvasSpace::World;
    CHECK(config.IsValid());
    config.castsWorldLight = true;
    CHECK(config.IsValid());
}

void TestWidgets() {
    using namespace ToolEngine::SpriteUi::Widgets;

    UISliderComponent slider;
    CHECK(slider.SetFromPointer(75.0f, 100.0f));
    CHECK(Near(slider.value, 0.75f));
    CHECK(slider.SetValue(5.0f));
    CHECK(Near(slider.value, 1.0f));

    slider.minValue = 10.0f;
    slider.maxValue = 0.0f;
    CHECK(!slider.SetValue(4.0f));

    UIScrollViewComponent scroll;
    scroll.contentSize = {1000, 1200};
    scroll.viewportSize = {400, 500};
    scroll.allowScrollX = true;
    scroll.allowScrollY = true;
    scroll.scrollOffset = {5000, 5000};
    scroll.ClampScroll();
    CHECK(Near(scroll.scrollOffset.x, 600.0f));
    CHECK(Near(scroll.scrollOffset.y, 700.0f));

    scroll.allowScrollY = false;
    scroll.ScrollBy({-100, -100});
    CHECK(Near(scroll.scrollOffset.y, 0.0f));
}

void TestTweenConflictPolicies() {
    using namespace ToolEngine::SpriteUi::Animation;
    ToonMotionSystem system;

    TweenTrack first;
    first.target = {77, 3};
    first.property = TweenProperty::Position;
    first.from = {0, 0, 0, 0};
    first.to = {10, 0, 0, 0};
    first.duration = 1.0f;
    first.easing = Easing::Linear;
    CHECK(system.Add(first));

    TweenTrack duplicateId = first;
    duplicateId.id = 1;
    CHECK(!system.Add(duplicateId, TweenConflict::Queue));

    TweenTrack ignored = first;
    ignored.from = {10, 0, 0, 0};
    ignored.to = {20, 0, 0, 0};
    CHECK(!system.Add(ignored, TweenConflict::Ignore));

    TweenTrack queued = ignored;
    queued.to = {30, 0, 0, 0};
    CHECK(system.Add(queued, TweenConflict::Queue));
    auto update = system.Update(1.0f);
    CHECK(update.completed.size() == 1);
    CHECK(update.samples.size() == 1);

    update = system.Update(0.5f);
    CHECK(update.samples.size() == 1);
    CHECK(Near(update.samples[0].value.x, 20.0f));

    TweenTrack replacement = first;
    replacement.to = {100, 0, 0, 0};
    CHECK(system.Add(replacement, TweenConflict::Override));
    CHECK(system.Size() == 1);
    update = system.Update(0.5f);
    CHECK(update.samples.size() == 1);
    CHECK(Near(update.samples[0].value.x, 50.0f));
}

void TestToonMotion() {
    using namespace ToolEngine::SpriteUi::Animation;

    ToonMotionSystem system;
    TweenTrack track;
    track.target = {7, 2};
    track.property = TweenProperty::Opacity;
    track.from = {0, 0, 0, 0};
    track.to = {1, 0, 0, 0};
    track.duration = 1.0f;
    track.easing = Easing::Linear;

    CHECK(system.Add(track));
    auto update = system.Update(0.5f);
    CHECK(update.samples.size() == 1);
    CHECK(Near(update.samples[0].value.x, 0.5f));
    CHECK(update.completed.empty());

    update = system.Update(0.5f);
    CHECK(update.completed.size() == 1);
    CHECK(Near(update.samples[0].value.x, 1.0f));
    CHECK(system.Size() == 0);

    TweenTrack zero = track;
    zero.duration = 0.0f;
    CHECK(system.Add(zero));
    update = system.Update(0.0f);
    CHECK(update.completed.size() == 1);

    const Vec2 shakeEnd = EvaluateShake(1.0f, 1.0f, {10, 10});
    CHECK(Near(shakeEnd.x, 0.0f));
    CHECK(Near(shakeEnd.y, 0.0f));
}

void TestRendererAbiPremultiplicationAndBatching() {
    using namespace ToolEngine::SpriteUi::Rendering;

    CHECK(sizeof(SpriteInstanceGPU) == 80);
    const Vec4 tint = SrgbStraightToLinearPremultiplied({1, 0, 0, 0.5f});
    CHECK(Near(tint.w, 0.5f));
    CHECK(Near(tint.x, 0.5f));
    CHECK(IsValidPremultipliedTint(tint));

    std::vector<UIRenderItem> items(3);
    for (std::size_t i = 0; i < items.size(); ++i) {
        items[i].handle = {static_cast<std::uint32_t>(i + 1), 1};
        items[i].layout = {{static_cast<float>(i * 20), 0}, {10, 10}, true, false};
        items[i].tintPremultipliedLinear = {1, 1, 1, 1};
        items[i].paintOrder = i;
        items[i].textureSlice = static_cast<std::uint32_t>(i == 1 ? 2 : 1);
    }

    RenderExtractor extractor;
    ExtractionStats stats;
    const ClipRectGPU clip{0.0f, 0.0f, 100.0f, 100.0f};
    items[0].clipIndex = 0;
    const auto packet = extractor.Build(44, items, &stats, std::span<const ClipRectGPU>(&clip, 1));
    CHECK(packet.instances.size() == 3);
    CHECK(packet.clipRects.size() == 1);
    // A-B-A must remain three batches: sorting by material would break alpha order.
    CHECK(packet.batches.size() == 3);
    CHECK(IsFramePacketValid(packet));
    CHECK(stats.submittedItems == 3);
    CHECK(packet.frameId == 44);

    std::vector<UIRenderItem> tied(2);
    tied[0].handle = {9, 1};
    tied[1].handle = {3, 1};
    for (auto& item : tied) {
        item.layout = {{0, 0}, {10, 10}, true, false};
        item.tintPremultipliedLinear = {1, 1, 1, 1};
        item.paintOrder = 7;
    }
    tied[0].layout.resolvedPosition.x = 90.0f;
    tied[1].layout.resolvedPosition.x = 30.0f;
    const FramePacket tiePacket = extractor.Build(46, tied);
    CHECK(tiePacket.instances.size() == 2);
    // Equal paint keys are made deterministic by the packed generational handle.
    CHECK(Near(tiePacket.instances[0].transformRow0[2], 30.0f));
    CHECK(tiePacket.batches.size() == 1);

    items[1].clipIndex = 3;
    const auto invalidClipPacket = extractor.Build(45, items, nullptr,
        std::span<const ClipRectGPU>(&clip, 1));
    CHECK(invalidClipPacket.instances.size() == 2);
    CHECK(IsFramePacketValid(invalidClipPacket));

    FramePacket corrupt = packet;
    corrupt.batches[1].firstInstance = 0;
    CHECK(!IsFramePacketValid(corrupt));

    FramePacket badTint = packet;
    badTint.instances[0].tint[3] = 2.0f;
    CHECK(!IsFramePacketValid(badTint));

    NullRenderBackend backend;
    CHECK(backend.Submit(packet));
    CHECK(!backend.Submit(corrupt));
    CHECK(backend.GetStats().submittedFrames == 1);
}

} // namespace

// Layout engine regression tests
#include "SpriteUi/Layout/Layout.hpp"
#include "SpriteUi/Layout/LayoutTree.hpp"
static void TestFlexLayout() {
    using namespace ToolEngine::SpriteUi;
    LayoutNode root; root.style.direction=LayoutDirection::Row; root.style.width={SizeMode::Fixed,300}; root.style.height={SizeMode::Fixed,100}; root.style.gap=10;
    LayoutNode a; a.style.width={SizeMode::Fixed,100}; a.style.height={SizeMode::Fixed,40};
    LayoutNode b; b.style.width={SizeMode::Flex,0}; b.style.height={SizeMode::Fixed,50}; b.style.grow=1;
    root.children={a,b}; LayoutEngine engine; LayoutStats stats;
    engine.Measure(root,{300,100},&stats); engine.Arrange(root,{{0,0},{300,100}},&stats);
    CHECK(root.children[0].rect.size.x==100.0f);
    CHECK(root.children[1].rect.size.x==190.0f);
    CHECK(root.children[1].rect.position.x==110.0f);
    CHECK(stats.measured==3 && stats.arranged==3);
}


static void TestLayoutTreeAndAlignment() {
    using namespace ToolEngine::SpriteUi;
    LayoutTree tree;
    FlexItem rootStyle;
    rootStyle.width={SizeMode::Fixed,200};
    rootStyle.height={SizeMode::Fixed,100};
    rootStyle.direction=LayoutDirection::Row;
    rootStyle.gap=10;
    const UIHandle root=tree.Create({},rootStyle);
    FlexItem childStyle;
    childStyle.width={SizeMode::Fixed,50};
    childStyle.height={SizeMode::Fixed,20};
    childStyle.alignItems=AlignItems::Center;
    const UIHandle child=tree.Create(root,childStyle);
    const UIHandle grandchild=tree.Create(child,childStyle);
    CHECK(root.IsValid() && child.IsValid() && grandchild.IsValid());
    CHECK(!tree.Create({123456u,1u},childStyle).IsValid());
    tree.Layout({{0,0},{200,100}});
    CHECK(Near(tree.Get(child)->node.rect.position.y,40.0f));
    CHECK(tree.Remove(child));
    CHECK(tree.Get(child)==nullptr);
    CHECK(tree.Get(grandchild)==nullptr);
    CHECK(tree.Get(root)!=nullptr);
    CHECK(!tree.Remove(child));
}


static void TestClipHierarchyAndInputClip() {
    using namespace ToolEngine::SpriteUi;
    using namespace ToolEngine::SpriteUi::Rendering;
    const ClipNode nodes[] = {
        {{0,0,100,100}, NoClipIndex, true},
        {{25,25,125,125}, 0, true},
        {{0,0,0,0}, 1, false}
    };
    const auto clips=BuildClipHierarchy(nodes);
    CHECK(clips.valid);
    CHECK(clips.effectiveRects.size()==2);
    CHECK(clips.nodeClipIndices[2]==clips.nodeClipIndices[1]);
    CHECK(Near(clips.effectiveRects[1].minX,25));
    CHECK(Near(clips.effectiveRects[1].maxX,100));
    const ClipNode invalid[] = {{{0,0,1,1},1,true}};
    CHECK(!BuildClipHierarchy(invalid).valid);

    Input::HitTarget target;
    target.handle={7,1};
    target.localSize={100,100};
    target.clipEnabled=true;
    target.clipRectLogical={{0,0},{50,50}};
    Input::UIInputSnapshot input;
    input.pointerPositionLogical={75,75};
    input.frameId=1;
    Input::UIEventRouter router;
    const auto events=router.Process(input,std::span<const Input::HitTarget>(&target,1));
    CHECK(events.empty());
}

int main() {
    TestFlexLayout();
    TestClipHierarchyAndInputClip();
    TestLayoutTreeAndAlignment();
    TestAnchorsAndTransforms();
    TestInputRouterClickAndCapture();
    TestInputRouterCancelAndOcclusion();
    TestHoverMoveEvent();
    TestInvalidInputCancelsCapture();
    TestHitTestTransformAndMaxFrameId();
    TestSpatialNavigationAndDirtyFlags();
    TestSemanticNavigationRouter();
    TestEventBubblingAndHoverConsumption();
    TestCanvasConfig();
    TestWidgets();
    TestToonMotion();
    TestTweenConflictPolicies();
    TestRendererAbiPremultiplicationAndBatching();

    if (failures != 0) {
        std::cerr << failures << " failure(s) in " << testsRun << " checks.\n";
        return EXIT_FAILURE;
    }
    std::cout << "All " << testsRun << " checks passed.\n";
    return EXIT_SUCCESS;
}


