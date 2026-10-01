#include "../src/water_fill.hpp"
#include <cassert>
#include <iostream>
int main(){
 Model empty;Comparison c;c.requestedStep=c.options.step=100;c.region={true,-1,1000,-1,1000,-1000,1000};Cargo cargo;
 for(int y=0;y<9;y++)for(int x=0;x<9;x++)empty.points.push_back({float(x*100+50),float(y*100+50),float(x==0||x==8||y==0||y==8?100:0)});
 auto a=waterFill(empty,c,cargo);assert(a.available&&a.cells==49&&a.level==100&&std::abs(a.volume-49000000)<1e-6);
 // A lower rim determines the spill level, not the average wall height.
 empty.points[4].z=60;a=waterFill(empty,c,cargo);assert(a.available&&a.level==60&&std::abs(a.volume-29400000)<1e-6);
 // An open gate cannot be treated as a closed tank.
 empty.points[4].z=0;a=waterFill(empty,c,cargo);assert(!a.available);
 empty.points[4].z=100;empty.points[40].z=NAN;
 // Remove one bracketed bed sample; the bounded interpolation restores it.
 empty.points.erase(empty.points.begin()+40);a=waterFill(empty,c,cargo);assert(a.available&&a.interpolated==1&&a.volume==49000000);
 auto before=empty.points;c.options.dz=230;a=waterFill(empty,c,cargo);assert(a.available&&a.level==330&&a.volume==49000000&&empty.points.size()==before.size());
 c.options.angle=180;c.options.dx=c.options.dy=900;c.options.dz=0;a=waterFill(empty,c,cargo);assert(a.available&&a.level==100&&a.volume==49000000);
 std::cout<<"PASS closed bed, spill notch, open gate, bounded hole, height/yaw invariance and immutable Empty\n";
}
