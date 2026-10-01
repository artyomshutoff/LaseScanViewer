#include "../src/volume.hpp"
#include <cassert>
#include <iostream>
Model plane(bool full,bool hole,bool open=false){Model m;for(int x=0;x<40;x++)for(int y=0;y<40;y++){
    if(hole&&x>=16&&x<23&&y>=(open?0:16)&&y<23)continue;
    for(int i=0;i<16;i++)m.points.push_back({x+.2f+float(i%4)*.15f,y+.2f+float(i/4)*.15f,float(.01*x+.02*y+(full?2:0))});
}
m.lo={.2f,.2f,0};m.hi={39.65f,39.65f,4};return m;}
int main(){
 auto a=plane(true,false),b=plane(false,true);CompareOptions o;o.aligned=true;o.step=1;o.adaptiveGrid=false;
 Region roi{true,0,40,0,40,-100,100};auto c=compareClouds(a,b,roi,o);
 assert(c.cells.size()==1600&&c.reconstructedArea==49);
 auto cargo=buildCargo(c,.1,true,&a,false,&b);assert(std::abs(cargo.volume-3200)<.1);
 assert(std::abs(cargo.reconstructedVolume-98)<.1);
 o.adaptiveGrid=true;auto supported=calculateVolume(a,b,roi,o,.1,true,false);
 assert(supported.comparison.adaptiveGridUsed&&supported.comparison.options.step==1.25);o.adaptiveGrid=false;
 o.reconstructGaps=false;auto observed=compareClouds(a,b,roi,o);assert(observed.cells.size()==1551&&observed.reconstructedArea==0);
 o.reconstructGaps=true;b=plane(false,true,true);c=compareClouds(a,b,roi,o);assert(!c.cells.count({19,10}));
 a=plane(true,true);b=plane(false,true);c=compareClouds(a,b,roi,o);assert(!c.cells.count({19,19})&&c.reconstructedArea==0);
 a=plane(true,false);b=plane(false,false);o.adaptiveGrid=true;int progress=-1;
 auto v=calculateVolume(a,b,roi,o,.1,true,false,[&](int n){assert(n>=progress);progress=n;});
 assert(progress==100&&v.comparison.adaptiveGridUsed&&v.comparison.options.step==.75);
 o.adaptiveGrid=false;v=calculateVolume(a,b,roi,o,.1,true,false);assert(!v.comparison.adaptiveGridUsed&&v.comparison.options.step==1);
 // Identical scans, including holes, must never manufacture cargo.
 b=plane(false,true);v=calculateVolume(b,b,roi,o,.1,true,false);assert(v.cargo.volume==0);
 std::cout<<"PASS enclosed sloping floor, accounting, open boundary, double-missing, adaptive/manual grids, self-zero, progress\n";
}
