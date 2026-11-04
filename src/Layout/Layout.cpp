#include "SpriteUi/Layout/Layout.hpp"
#include <algorithm>
#include <cmath>
namespace ToolEngine::SpriteUi {
namespace {
float Resolve(Length l,float available,float measured) {
 switch(l.mode) {
 case SizeMode::Fixed: return std::max(0.0f,l.value);
 case SizeMode::Percent: return std::max(0.0f,available*l.value);
 case SizeMode::Flex: return std::max(0.0f,measured);
 case SizeMode::Auto: default: return std::max(0.0f,measured);
 }
}
float ClampSize(float x,float lo,float hi) {
 if(!std::isfinite(x)) x=0;
 return std::clamp(x,std::max(0.0f,lo),std::max(std::max(0.0f,lo),hi));
}
}
Vec2 LayoutEngine::Measure(LayoutNode& n,Vec2 a,LayoutStats* s) const { return MeasureImpl(n,a,s); }
Vec2 LayoutEngine::MeasureImpl(LayoutNode& n,Vec2 a,LayoutStats* s) const {
 if(s) ++s->measured;
 float main=0,cross=0;
 const bool row=n.style.direction==LayoutDirection::Row;
 for(auto& c:n.children) {
  Vec2 m=MeasureImpl(c,a,s);
  float cm=row?m.x:m.y, cc=row?m.y:m.x;
  float margins=row?c.style.margin.left+c.style.margin.right:c.style.margin.top+c.style.margin.bottom;
  float crossMargins=row?c.style.margin.top+c.style.margin.bottom:c.style.margin.left+c.style.margin.right;
  main+=cm+margins; cross=std::max(cross,cc+crossMargins);
 }
 if(!n.children.empty()) main+=n.style.gap*static_cast<float>(n.children.size()-1);
 Vec2 content=row?Vec2{main,cross}:Vec2{cross,main};
 content.x+=n.style.padding.left+n.style.padding.right;
 content.y+=n.style.padding.top+n.style.padding.bottom;
 n.measured={ClampSize(Resolve(n.style.width,a.x,content.x),n.style.minSize.x,n.style.maxSize.x),
             ClampSize(Resolve(n.style.height,a.y,content.y),n.style.minSize.y,n.style.maxSize.y)};
 return n.measured;
}
void LayoutEngine::Arrange(LayoutNode& n,Rect b,LayoutStats* s) const { ArrangeImpl(n,b,s); }
void LayoutEngine::ArrangeImpl(LayoutNode& n,Rect b,LayoutStats* s) const {
 if(s) ++s->arranged;
 n.rect=b;
 if(n.children.empty()) return;
 const bool row=n.style.direction==LayoutDirection::Row;
 float innerMain=(row?b.size.x:b.size.y)-(row?n.style.padding.left+n.style.padding.right:n.style.padding.top+n.style.padding.bottom);
 float innerCross=(row?b.size.y:b.size.x)-(row?n.style.padding.top+n.style.padding.bottom:n.style.padding.left+n.style.padding.right);
 innerMain=std::max(0.0f,innerMain); innerCross=std::max(0.0f,innerCross);
 float fixed= n.style.gap*static_cast<float>(n.children.size()-1), totalGrow=0,totalShrink=0;
 std::vector<float> bases; bases.reserve(n.children.size());
 for(auto& c:n.children) {
  float base=row?c.measured.x:c.measured.y;
  float margin=row?c.style.margin.left+c.style.margin.right:c.style.margin.top+c.style.margin.bottom;
  base=std::max(0.0f,base); bases.push_back(base); fixed+=base+margin;
  totalGrow+=std::max(0.0f,c.style.grow);
  totalShrink+=std::max(0.0f,c.style.shrink)*base;
 }
 float free=innerMain-fixed;
 float cursor=(row?b.position.x+n.style.padding.left:b.position.y+n.style.padding.top);
 for(size_t i=0;i<n.children.size();++i) {
  auto& c=n.children[i];
  float before=row?c.style.margin.left:c.style.margin.top;
  float after=row?c.style.margin.right:c.style.margin.bottom;
  float main=bases[i];
  if(free>0 && totalGrow>0) main+=free*std::max(0.0f,c.style.grow)/totalGrow;
  else if(free<0 && totalShrink>0) main+=free*(std::max(0.0f,c.style.shrink)*bases[i])/totalShrink;
  main=std::max(0.0f,main);
  float crossSize=std::min(innerCross,row?c.measured.y:c.measured.x);
  float crossBefore=row?c.style.margin.top:c.style.margin.left;
  float crossAfter=row?c.style.margin.bottom:c.style.margin.right;
  crossSize=std::max(0.0f,innerCross-crossBefore-crossAfter);
  cursor+=before;
  Rect cb;
  if(row) cb={{cursor,b.position.y+n.style.padding.top+crossBefore},{main,crossSize}};
  else cb={{b.position.x+n.style.padding.left+crossBefore,cursor},{crossSize,main}};
  ArrangeImpl(c,cb,s);
  cursor+=main+after+n.style.gap;
 }
}
} // namespace
