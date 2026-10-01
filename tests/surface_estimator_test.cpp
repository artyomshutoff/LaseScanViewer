#include "../src/rim_alignment.hpp"
#include <cassert>
#include <iostream>
void boundsFor(Model& m){m.lo=m.hi=m.points.front();for(auto p:m.points){m.lo.x=std::min(m.lo.x,p.x);m.lo.y=std::min(m.lo.y,p.y);m.lo.z=std::min(m.lo.z,p.z);m.hi.x=std::max(m.hi.x,p.x);m.hi.y=std::max(m.hi.y,p.y);m.hi.z=std::max(m.hi.z,p.z);}}
int main(){
 HeightCell mixed;for(int i=0;i<30;i++)mixed.add(10);for(int i=0;i<70;i++)mixed.add(100);assert(mixed.value(4)==10&&mixed.value(3)==100);
 HeightCell outlier;outlier.add(-999);for(int i=0;i<20;i++)outlier.add(10);assert(outlier.value(4)==10);
 HeightCell sparse;for(double v:{1.,5.,99.})sparse.add(v);assert(sparse.value(4)==5);
 HeightCell interpolation;for(int i=0;i<20;i++)interpolation.add(i);assert(std::abs(interpolation.value(4)-1.9)<1e-9);
 Model a,b;for(int x=0;x<40;x++)for(int y=0;y<20;y++)for(int n=0;n<4;n++){b.points.push_back({float(x*100+n),float(y*100+n),1000});a.points.push_back({float(x*100+n),float(y*100+n),900});}boundsFor(a);boundsFor(b);
 assert(std::abs(rimHeightCorrection(a,b,{})+100)<1e-6);
 auto partial=a;partial.points.erase(std::remove_if(partial.points.begin(),partial.points.end(),[](Point p){return p.x>1000;}),partial.points.end());boundsFor(partial);assert(rimHeightCorrection(partial,b,{})==0);
 for(auto& p:a.points)p.z=1600;boundsFor(a);assert(rimHeightCorrection(a,b,{})==0);
 std::cout<<"PASS lower surface quantile, sparse fallback, isolated outlier, exact rim shift, insufficient anchor extent and unrelated height rejection\n";
}
