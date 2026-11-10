// Camada L3 (docking, interacao): roteiros de eventos do ponteiro sobre o controlador de arrasto.
// Executavel proprio, sem o harness principal.

#include "SpriteUi/Widgets/DockInteraction.hpp"

#include <array>
#include <cstdio>

namespace {

using namespace ToolEngine::SpriteUi;
using namespace ToolEngine::SpriteUi::Widgets;

int run = 0;
int failed = 0;
const char* current = "";

void Check(bool ok, const char* expr) {
    ++run;
    if (!ok) {
        ++failed;
        std::fprintf(stderr, "[FAIL] %s: %s\n", current, expr);
    }
}
#define CHECK(expr) Check((expr), #expr)

// Duas folhas lado a lado em uma tela 200x100: A a esquerda, B a direita.
const std::array<DockLeafRect, 2> kLeaves = {
    DockLeafRect{UIHandle{1, 1}, Rect{{0, 0}, {100, 100}}},
    DockLeafRect{UIHandle{2, 1}, Rect{{100, 0}, {100, 100}}},
};
const UIHandle kLeafA = UIHandle{1, 1};
const UIHandle kWindow = UIHandle{9, 1};

void TestDropSlotZones() {
    current = "TestDropSlotZones";
    const Rect leaf{{0, 0}, {100, 100}};
    CHECK(ComputeDropSlot(leaf, {50, 50}, 0.25f) == DockSlot::Center);
    CHECK(ComputeDropSlot(leaf, {5, 50}, 0.25f) == DockSlot::Left);
    CHECK(ComputeDropSlot(leaf, {95, 50}, 0.25f) == DockSlot::Right);
    CHECK(ComputeDropSlot(leaf, {50, 5}, 0.25f) == DockSlot::Top);
    CHECK(ComputeDropSlot(leaf, {50, 95}, 0.25f) == DockSlot::Bottom);
    CHECK(ComputeDropSlot(leaf, {150, 50}, 0.25f) == DockSlot::None);        // fora da folha
    CHECK(ComputeDropSlot(Rect{{0, 0}, {0, 0}}, {0, 0}, 0.25f) == DockSlot::None); // sem area
    CHECK(ComputeDropSlot(leaf, {50, 50}, 0.9f) == DockSlot::None);          // fracao invalida
}

void TestClickWithoutMoveDoesNothing() {
    current = "TestClickWithoutMoveDoesNothing";
    DockDragController c;
    c.BeginPress(kWindow, {20, 20});
    CHECK(c.IsPressed());
    CHECK(!c.IsDragging());
    const DockDropResult r = c.Release({20, 20}, kLeaves);
    CHECK(r.action == DockAction::None);
    CHECK(!c.IsPressed());
}

void TestJitterBelowThresholdIsNotADrag() {
    current = "TestJitterBelowThresholdIsNotADrag";
    DockDragController c;
    c.BeginPress(kWindow, {20, 20});
    CHECK(!c.Move({23, 20}));          // 3 px, abaixo do limiar de 6 px
    CHECK(!c.Move({21, 22}));
    CHECK(!c.IsDragging());
    CHECK(c.Release({21, 22}, kLeaves).action == DockAction::None);
}

void TestDragToLeftEdgeDocks() {
    current = "TestDragToLeftEdgeDocks";
    DockDragController c;
    c.BeginPress(kWindow, {50, 50});
    CHECK(c.Move({60, 50}));           // 10 px: arrasto ativo
    CHECK(c.IsDragging());
    const DockDropResult r = c.Release({5, 50}, kLeaves);
    CHECK(r.action == DockAction::Dock);
    CHECK(r.slot == DockSlot::Left);
    CHECK(r.targetLeaf == kLeafA);
}

void TestDragToCenterTabs() {
    current = "TestDragToCenterTabs";
    DockDragController c;
    c.BeginPress(kWindow, {150, 50});
    c.Move({140, 50});
    const DockDropResult r = c.Release({150, 50}, kLeaves);
    CHECK(r.action == DockAction::Tab);
    CHECK(r.targetLeaf == UIHandle(2, 1));
}

void TestDragOutsideFloats() {
    current = "TestDragOutsideFloats";
    DockDragController c;
    c.BeginPress(kWindow, {50, 50});
    c.Move({400, 300});
    CHECK(c.Release({400, 300}, kLeaves).action == DockAction::Float);
}

void TestCancelDropsTheGesture() {
    current = "TestCancelDropsTheGesture";
    DockDragController c;
    c.BeginPress(kWindow, {50, 50});
    c.Move({80, 50});
    c.Cancel();
    CHECK(!c.IsPressed());
    CHECK(!c.IsDragging());
    // Soltar depois do cancelamento nao pode produzir acao.
    CHECK(c.Release({5, 50}, kLeaves).action == DockAction::None);
}

void TestNoSpontaneousMovementWithoutInput() {
    current = "TestNoSpontaneousMovementWithoutInput";
    DockDragController c;
    c.BeginPress(kWindow, {50, 50});
    // Mil quadros com o ponteiro parado: o gesto nao vira arrasto sozinho.
    bool started = false;
    for (int frame = 0; frame < 1000; ++frame) started = started || c.Move({50, 50});
    CHECK(!started);
    CHECK(!c.IsDragging());
    CHECK(c.Release({50, 50}, kLeaves).action == DockAction::None);
}

void TestGestureResetsAfterRelease() {
    current = "TestGestureResetsAfterRelease";
    DockDragController c;
    c.BeginPress(kWindow, {50, 50});
    c.Move({90, 50});
    (void)c.Release({5, 50}, kLeaves);
    c.BeginPress(kWindow, {50, 50});
    CHECK(!c.IsDragging());            // novo gesto comeca limpo
    CHECK(c.Release({50, 50}, kLeaves).action == DockAction::None);
}

void TestClampKeepsWindowOnScreen() {
    current = "TestClampKeepsWindowOnScreen";
    const Rect screen{{0, 0}, {200, 100}};
    const Rect off = ClampWindowToScreen(Rect{{-50, 150}, {80, 40}}, screen);
    CHECK(off.position.x == 0.0f);
    CHECK(off.position.y == 60.0f);    // 100 - 40
    CHECK(off.size.x == 80.0f && off.size.y == 40.0f);

    const Rect big = ClampWindowToScreen(Rect{{10, 10}, {500, 300}}, screen);
    CHECK(big.size.x == 200.0f && big.size.y == 100.0f);
    CHECK(big.position.x == 0.0f && big.position.y == 0.0f);

    const Rect inside = ClampWindowToScreen(Rect{{30, 20}, {80, 40}}, screen);
    CHECK(inside.position.x == 30.0f && inside.position.y == 20.0f);
}

} // namespace

int main() {
    TestDropSlotZones();
    TestClickWithoutMoveDoesNothing();
    TestJitterBelowThresholdIsNotADrag();
    TestDragToLeftEdgeDocks();
    TestDragToCenterTabs();
    TestDragOutsideFloats();
    TestCancelDropsTheGesture();
    TestNoSpontaneousMovementWithoutInput();
    TestGestureResetsAfterRelease();
    TestClampKeepsWindowOnScreen();
    std::printf("dock interaction: %d/%d checks passed\n", run - failed, run);
    return failed == 0 ? 0 : 1;
}
