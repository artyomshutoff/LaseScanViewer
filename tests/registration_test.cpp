#include "../src/registration.hpp"
#include <iostream>
#include <cassert>
void boundsOf(Model& m){m.lo=m.hi=m.points[0];for(auto p:m.points){m.lo.x=std::min(m.lo.x,p.x);m.lo.y=std::min(m.lo.y,p.y);m.lo.z=std::min(m.lo.z,p.z);m.hi.x=std::max(m.hi.x,p.x);m.hi.y=std::max(m.hi.y,p.y);m.hi.z=std::max(m.hi.z,p.z);}}
int main(){Model base;base.selectedType=1;for(int x=0;x<64;x++)for(int y=0;y<36;y++){float z=float(300+80*std::sin(x*.15)+50*std::cos(y*.22)+(x>49?700:0)+(y<4?250:0));base.points.push_back({float(x*40),float(y*40),z});}boundsOf(base);
for(double angle:{180.,37.})for(bool disturbed:{false,true}){CompareOptions truth;truth.angle=angle;truth.dx=1800;truth.dy=-700;truth.dz=110;Model active;active.selectedType=1;for(auto p:base.points){if(p.x>650&&p.x<1450&&p.y>350&&p.y<1050)p.z+=500;active.points.push_back(transformBase(p,truth));}if(disturbed){std::vector<Point> sparse;size_t index=0;for(auto p:active.points){size_t i=index++;if(i%11==7)continue;p.x+=float(int(i%7)-3)*.5f;p.y+=float(int(i%9)-4)*.4f;p.z+=float(int(i%5)-2)*.7f;sparse.push_back(p);}active.points=std::move(sparse);}boundsOf(active);auto r=registration::align(active,base,{});std::cerr<<angle<<": "<<r.options.angle<<" xyz="<<r.options.dx<<","<<r.options.dy<<","<<r.options.dz<<" overlap="<<r.overlap<<"\n";assert(r.reliable);assert(std::abs(std::remainder(r.options.angle-angle,360.))<1);assert(std::abs(r.options.dx-truth.dx)<20&&std::abs(r.options.dy-truth.dy)<20&&std::abs(r.options.dz-truth.dz)<20);}
Model lowerVisible=base,loaded;lowerVisible.kind=1;loaded.kind=2;loaded.selectedType=1;CompareOptions known;known.angle=180;known.dx=500;known.dy=300;known.dz=80;
for(auto p:base.points){if(p.x>650&&p.x<1450&&p.y>350&&p.y<1050)p.z+=500;loaded.points.push_back(transformBase(p,known));}
for(int i=0;i<40;i++)lowerVisible.points.push_back({float(30+i*50),20,-1200});boundsOf(lowerVisible);boundsOf(loaded);auto occluded=registration::align(loaded,lowerVisible,{});std::cerr<<"occluded "<<occluded.options.angle<<" dz "<<occluded.options.dz<<"\n";assert(std::abs(std::remainder(occluded.options.angle-180,360.))<1);assert(std::abs(occluded.options.dz-80)<20);
Model flat=base;for(auto& p:flat.points)p.z=0;boundsOf(flat);bool rejected=false;try{registration::align(flat,flat,{});}catch(...){rejected=true;}assert(rejected);
std::cout<<"PASS: reversed and oblique scans with changed cargo\n";
}

