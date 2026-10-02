#include "../src/bed_dimensions.hpp"
#include <cassert>
#include <iostream>
int main(){
 for(double angle:{0.,37.,90.,180.}){
  WaterFill w;w.available=true;w.level=2400;double a=angle*3.141592653589793/180;
  for(int y=0;y<20;++y)for(int x=0;x<60;++x){double X=x*100,Y=y*100;for(int j=0;j<8;++j){double xx=X+((j%4==1||j%4==2)?100:0),yy=Y+((j%4>=2)?100:0);w.geometry.points.push_back({float(xx*std::cos(a)-yy*std::sin(a)+1300),float(xx*std::sin(a)+yy*std::cos(a)-2000),j<4?400.f:2400.f});}}
  auto d=measureBed(w);assert(d.available&&std::abs(d.length-6000)<.01&&std::abs(d.width-2000)<.01&&d.height==2000);
  w.available=false;assert(!measureBed(w).available);
 }
 assert(!measureBed({}).available);std::cout<<"PASS rotated interior dimensions, height and unavailable scans\n";
}
