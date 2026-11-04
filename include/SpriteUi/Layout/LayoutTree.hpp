#pragma once
#include "SpriteUi/Layout/Layout.hpp"
#include "SpriteUi/Core/Handle.hpp"
#include <optional>
#include <unordered_map>
namespace ToolEngine::SpriteUi {
struct LayoutElement { UIHandle id{}; UIHandle parent{}; LayoutNode node{}; std::vector<UIHandle> children{}; bool visible=true; };
class LayoutTree {
public:
 UIHandle Create(UIHandle parent={},FlexItem style={});
 bool Remove(UIHandle id);
 LayoutElement* Get(UIHandle id);
 const LayoutElement* Get(UIHandle id) const;
 bool SetStyle(UIHandle id,FlexItem style);
 bool SetVisible(UIHandle id,bool visible);
 void Layout(Rect viewport,LayoutStats* stats=nullptr);
 const std::vector<UIHandle>& Roots() const noexcept { return roots_; }
private:
 UIHandle next_{0,1};
 std::unordered_map<uint64_t,LayoutElement> elements_;
 std::vector<UIHandle> roots_;
 LayoutNode Build(UIHandle id);
 void Commit(UIHandle id,const LayoutNode& node);
 void RemoveSubtree(UIHandle id);
};
} // namespace
