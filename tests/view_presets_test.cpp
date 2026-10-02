#include "../src/view_presets.hpp"
#include <cassert>
#include <iostream>
int main(){
 for(double angle:{0.,25.,90.,145.,180.,-65.}){
  Model m;double a=angle*3.141592653589793/180;
  for(int x=-50;x<=50;x++)for(int y=-10;y<=10;y++)m.points.push_back({float(x*std::cos(a)-y*std::sin(a)),float(x*std::sin(a)+y*std::cos(a)),float(y%3)});
  double h=view::heading(m),r=h*3.141592653589793/180;
  assert(std::abs(std::sin(r-a))<.02); // Trimming in scanner XY can bias PCA slightly.
  auto f=view::preset(view::Preset::Front,h),s=view::preset(view::Preset::Side,h);
  // Long axis disappears horizontally in front; occupies screen X in side.
  assert(std::abs(std::cos(r+f.yaw*3.141592653589793/180))<1e-10);
  assert(std::abs(std::cos(r+s.yaw*3.141592653589793/180)-1)<1e-10);
  assert(f.pitch==90&&s.pitch==90);
  m.lo={-100,-100,-10};m.hi={100,100,10};auto fitted=view::fit(m,f,m.lo,m.hi,1.6);assert(fitted.zoom>1&&std::abs(fitted.x)<.001&&std::abs(fitted.y)<.01);
 }
 assert(view::heading({})==0);auto t=view::preset(view::Preset::Top,35);assert(t.yaw==0&&t.pitch==0);
 std::cout<<"PASS front: width/Z, side: length/Z, rotated scans and empty input\n";
}
