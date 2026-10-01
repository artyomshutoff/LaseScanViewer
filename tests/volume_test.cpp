#include "../src/volume.hpp"
#include <cassert>
#include <iostream>
Model surface(bool raised){Model m;m.selectedType=0;for(int x=0;x<12;x++)for(int y=0;y<12;y++)m.points.push_back({x+.5f,y+.5f,float(1+(raised?2:0))});m.lo={.5f,.5f,1};m.hi={11.5f,11.5f,3};return m;}
void equal(double a,double b){if(std::abs(a-b)>1e-7)throw std::runtime_error("Volume mismatch");}
int main(){
 auto a=surface(true),b=surface(false);CompareOptions o;o.step=1;o.aligned=true;Region roi{true,0,12,0,12,-1000,1000};
 b.points.erase(std::remove_if(b.points.begin(),b.points.end(),[](Point p){return int(p.x)==5&&int(p.y)==5;}),b.points.end());
 auto c=compareClouds(a,b,roi,o);equal(c.positive,288);equal(c.reconstructedArea,1);auto g=buildCargo(c,.1,true,&a,true,&b);equal(g.volume,288);equal(g.reconstructedVolume,2);
 o.reconstructGaps=false;c=compareClouds(a,b,roi,o);equal(c.positive,286);equal(c.reconstructedArea,0);o.reconstructGaps=true;
 a.points.erase(std::remove_if(a.points.begin(),a.points.end(),[](Point p){return int(p.x)==5&&int(p.y)==5;}),a.points.end());c=compareClouds(a,b,roi,o);equal(c.positive,286);equal(c.reconstructedArea,0);
 a=surface(true);b=surface(false);b.points.erase(std::remove_if(b.points.begin(),b.points.end(),[](Point p){return p.x>=4&&p.x<8;}),b.points.end());c=compareClouds(a,b,roi,o);equal(c.reconstructedArea,0);equal(c.positive,192);
 // A visible internal rib/steep floor is not an exclusion mask for cargo.
 a=surface(true);b=surface(false);for(size_t i=0;i<a.points.size();i++){float z=int(a.points[i].x)==5?15.f:0.f;a.points[i].z+=z;b.points[i].z+=z;}
 c=compareClouds(a,b,roi,o);g=buildCargo(c,.1,true,&a,true,&b);equal(g.volume,288);
 // A small residual on the same steep rib in both scans is still excluded.
 for(size_t i=0;i<a.points.size();i++)if(int(a.points[i].x)==5)a.points[i].z=b.points[i].z+.15f;
 c=compareClouds(a,b,roi,o);g=buildCargo(c,.1,false,&a,true,&b);equal(g.volume,264);
 // Never interpolate through a discontinuity from low to high floor.
 a=surface(true);b=surface(false);for(auto& p:b.points)if(p.x>=6)p.z=100;
 b.points.erase(std::remove_if(b.points.begin(),b.points.end(),[](Point p){return int(p.x)==5;}),b.points.end());c=compareClouds(a,b,roi,o);equal(c.reconstructedArea,0);
 a=surface(true);b=surface(false);int progress=-1,calls=0;auto result=calculateVolume(a,b,roi,o,.1,true,true,[&](int n){assert(n>=progress&&n<=100);progress=n;++calls;});equal(result.cargo.volume,288);assert(progress==100&&calls>=8&&result.comparison.sensitivityChecked);
 auto self=calculateVolume(b,b,roi,o,.1,true,true);equal(self.cargo.volume,0);
 o.aligned=false;o.alignmentOverlap=.373;
 auto tentative=calculateVolume(a,b,roi,o,.1,true,true);equal(tentative.cargo.volume,288);
 assert(tentative.comparison.options.provisional&&!tentative.comparison.options.aligned&&!tentative.comparison.warning.empty());
 auto negative=calculateVolume(b,a,roi,o,.1,true,true);assert(negative.comparison.warning.find("Совмещение ненадёжно")!=std::string::npos);
 o.alignmentOverlap=-1;bool rejected=false;try{calculateVolume(a,b,roi,o,.1,true,true);}catch(...){rejected=true;}assert(rejected);
 std::cout<<"PASS: short supported holes, no extrapolation, no double-missing fill, wide gaps, discontinuities, internal ribs, interpolation accounting, five resolutions and progress\n";
}
