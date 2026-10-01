#include "../src/cargo.hpp"
#include <cassert>
#include <iostream>
int main(){
 Comparison c;c.region={true,0,500,0,500,-1000,3000};c.options.step=100;c.options.aligned=true;
 Model a,b;
 for(int x=0;x<5;x++)for(int y=0;y<5;y++){
  c.cells[{x,y}]={1000,0,1000,10000,false};
  a.points.push_back({float(x*100+50),float(y*100+50),1000});
  b.points.push_back({float(x*100+50),float(y*100+50),0});
 }
 // Tall rail and exterior wall in the SAME positive XY column as real cargo.
 a.points.push_back({250,250,1800});a.points.push_back({250,250,400});
 // Wall crossing the cargo surface level, present in the aligned empty scan.
 a.points.push_back({5,250,1000});b.points.push_back({5,250,1000});
 auto old=buildCargo(c,50,true,&a,false,&b),filtered=buildCargo(c,50,true,&a,true,&b);
 assert(old.cloud.points.size()==28&&filtered.cloud.points.size()==24);
 assert(filtered.bodyPointsExcluded==4); // includes adjacent cargo within tolerance
 assert(old.volume==filtered.volume&&old.geometry.faces.size()==filtered.geometry.faces.size());
 // Raised material away from the measured body survives even with steep sides.
 for(auto p:filtered.cloud.points)assert(p.z==1000);
 auto options=c.options;options.dx=250;Model empty;empty.points={{0,0,100}};
 BodyObservations shifted(empty,options,20);assert(shifted.matches({250,0,100}));assert(!shifted.matches({0,0,100}));assert(!shifted.matches({250,0,121}));
 std::cout<<"PASS body points at/above/below cargo, shifted baseline, preserved integral, filter toggle\n";
}
