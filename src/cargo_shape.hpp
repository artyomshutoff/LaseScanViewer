#pragma once
#include "cargo.hpp"
// Integral shape/support descriptors. They describe measured geometry only;
// no file identity, vehicle label or reference volume enters inference.
inline std::array<double,10> cargoShapeFeatures(const Comparison& c,const Cargo& g){
 std::array<double,10> f{};if(g.volume<=0)return f;
 std::set<CellKey> chosen;double area=0,d1=0,d2=0,b1=0,b2=0,a1=0,a2=0,perimeter=0;
 for(size_t i=0;i<g.geometry.points.size();i+=8){auto p=g.geometry.points[i];CellKey k{int(std::llround((p.x-c.region.x0)/c.options.step)),int(std::llround((p.y-c.region.y0)/c.options.step))};auto it=c.cells.find(k);if(it==c.cells.end())continue;chosen.insert(k);auto v=it->second;area+=v.area;d1+=v.delta*v.area;d2+=v.delta*v.delta*v.area;b1+=v.base*v.area;b2+=v.base*v.base*v.area;a1+=v.active*v.area;a2+=v.active*v.active*v.area;}
 for(auto k:chosen)for(auto d:std::initializer_list<CellKey>{{-1,0},{1,0},{0,-1},{0,1}})if(!chosen.count({k.first+d.first,k.second+d.second}))perimeter+=c.options.step;
 auto sd=[&](double x,double xx){return area>0?std::sqrt(std::max(0.,xx/area-x*x/(area*area)))/1000:0;};
 f[0]=area/1e8;f[1]=area>0?perimeter/std::sqrt(area):0;f[2]=sd(d1,d2);f[3]=sd(b1,b2);f[4]=sd(a1,a2);f[5]=g.reconstructedVolume/g.volume;f[6]=c.coverage();f[7]=c.options.bedHeightSpread/1000;
 double x=g.geometry.hi.x-g.geometry.lo.x,y=g.geometry.hi.y-g.geometry.lo.y;f[8]=std::max(x,y)/std::max(1.,std::min(x,y));f[9]=area>0?d1/area/1000:0;return f;
}
