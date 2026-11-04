#include "SpriteUi/Layout/LayoutTree.hpp"
#include <algorithm>
namespace ToolEngine::SpriteUi {
UIHandle LayoutTree::Create(UIHandle parent,FlexItem style) {
 UIHandle id=next_; if(++next_.index==0) ++next_.generation;
 LayoutElement e; e.id=id;e.parent=parent;e.node.style=style;
 elements_.emplace(id.Pack(),std::move(e));
 if(parent.IsValid() && elements_.contains(parent.Pack())) elements_[parent.Pack()].node.children.emplace_back();
 else roots_.push_back(id);
 return id;
}
bool LayoutTree::Remove(UIHandle id) {
 auto it=elements_.find(id.Pack()); if(it==elements_.end()) return false;
 for(auto& c:it->second.node.children) (void)c;
 for(auto iter=elements_.begin();iter!=elements_.end();) {
  if(iter->second.parent==id) { UIHandle child=iter->second.id; iter=elements_.erase(iter); (void)child; }
  else ++iter;
 }
 elements_.erase(it);
 std::erase(roots_,id);
 return true;
}
LayoutElement* LayoutTree::Get(UIHandle id) { auto i=elements_.find(id.Pack());return i==elements_.end()?nullptr:&i->second; }
const LayoutElement* LayoutTree::Get(UIHandle id) const { auto i=elements_.find(id.Pack());return i==elements_.end()?nullptr:&i->second; }
bool LayoutTree::SetStyle(UIHandle id,FlexItem s){auto* e=Get(id);if(!e)return false;e->node.style=s;return true;}
bool LayoutTree::SetVisible(UIHandle id,bool v){auto* e=Get(id);if(!e)return false;e->visible=v;return true;}
LayoutNode LayoutTree::Build(UIHandle id) {
 auto* e=Get(id); if(!e)return {};
 LayoutNode n; n.style=e->node.style;
 // Children are represented by parent references; discover deterministically by handle order.
 std::vector<UIHandle> children;
 for(auto& [k,v]:elements_) if(v.parent==id && v.visible) children.push_back(v.id);
 std::sort(children.begin(),children.end(),[](UIHandle a,UIHandle b){return a.Pack()<b.Pack();});
 for(auto c:children)n.children.push_back(Build(c));
 return n;
}
void LayoutTree::Commit(UIHandle id,const LayoutNode& n) {
 auto* e=Get(id);if(!e)return;e->node.rect=n.rect;e->node.measured=n.measured;
 std::vector<UIHandle> children;
 for(auto& [k,v]:elements_)if(v.parent==id&&v.visible)children.push_back(v.id);
 std::sort(children.begin(),children.end(),[](UIHandle a,UIHandle b){return a.Pack()<b.Pack();});
 for(size_t i=0;i<children.size()&&i<n.children.size();++i)Commit(children[i],n.children[i]);
}
void LayoutTree::Layout(Rect viewport,LayoutStats* stats) {
 LayoutEngine engine;
 for(auto id:roots_) { auto* e=Get(id);if(!e||!e->visible)continue;auto n=Build(id);engine.Measure(n,viewport.size,stats);engine.Arrange(n,viewport,stats);Commit(id,n); }
}
} // namespace
