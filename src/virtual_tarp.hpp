#pragma once
#include "water_fill.hpp"
struct VirtualTarp {Model geometry,cloud;double volume=0,level=0;size_t cells=0;bool available=false;std::string note;};
// Horizontal rim plane clips the already selected cargo prisms. This is a
// geometric excess, independent of any correction applied to the total volume.
inline VirtualTarp virtualTarp(const Cargo& cargo,const WaterFill& capacity,const Comparison* comparison=nullptr){
 VirtualTarp out;out.level=capacity.level;
 if(!capacity.available||!std::isfinite(capacity.level)){out.note="Уровень бортов не определён: требуется Empty и расчёт по воде";return out;}
 out.available=true;out.note="Геометрический объём выделенного груза выше горизонтального уровня бортов Empty; тент без провисания. Общий объём не изменяется.";
 for(size_t i=0;i+7<cargo.geometry.points.size();i+=8){auto lower=cargo.geometry.points[i],upper=cargo.geometry.points[i+4];double bottom=std::max(double(lower.z),out.level),top=upper.z;
  double area=double(cargo.geometry.points[i+1].x-lower.x)*double(cargo.geometry.points[i+3].y-lower.y);
  if(comparison){Point center{(lower.x+cargo.geometry.points[i+1].x)*.5f,(lower.y+cargo.geometry.points[i+3].y)*.5f,0};auto cell=comparison->cells.find(comparison->key(center));if(cell!=comparison->cells.end()){bottom=std::max(cell->second.base,out.level);top=cell->second.active;area=cell->second.area;}}
  if(top<=bottom)continue;
  if(area<=0)continue;uint32_t n=uint32_t(out.geometry.points.size());
  for(int j=0;j<8;j++){auto p=cargo.geometry.points[i+j];if(j<4)p.z=float(bottom);out.geometry.points.push_back(p);}
  for(auto f:std::initializer_list<std::array<uint32_t,3>>{{0,2,1},{0,3,2},{4,5,6},{4,6,7},{0,1,5},{0,5,4},{1,2,6},{1,6,5},{2,3,7},{2,7,6},{3,0,4},{3,4,7}})out.geometry.faces.push_back({n+f[0],n+f[1],n+f[2]});
  out.volume+=(top-bottom)*area;++out.cells;
 }
 for(auto p:cargo.cloud.points)if(p.z>out.level)out.cloud.points.push_back(p);
 if(!out.geometry.points.empty()){auto& m=out.geometry;m.lo=m.hi=m.points.front();for(auto p:m.points){m.lo.x=std::min(m.lo.x,p.x);m.lo.y=std::min(m.lo.y,p.y);m.lo.z=std::min(m.lo.z,p.z);m.hi.x=std::max(m.hi.x,p.x);m.hi.y=std::max(m.hi.y,p.y);m.hi.z=std::max(m.hi.z,p.z);}}
 out.cloud.lo=out.geometry.lo;out.cloud.hi=out.geometry.hi;return out;
}
