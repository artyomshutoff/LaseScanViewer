#include "../src/virtual_tarp.hpp"
#include <cassert>
#include <iostream>
int main(){Cargo c;c.geometry.points={{0,0,0},{2,0,0},{2,3,0},{0,3,0},{0,0,5},{2,0,5},{2,3,5},{0,3,5}};c.cloud.points={{1,1,2},{1,1,3},{1,1,4},{1,1,5}};WaterFill w;assert(!virtualTarp(c,w).available);w.available=true;w.level=3;auto t=virtualTarp(c,w);assert(t.available&&t.volume==12&&t.cells==1&&t.cloud.points.size()==2);for(auto p:t.geometry.points)assert(p.z>=3);assert(c.geometry.points[0].z==0);w.level=6;t=virtualTarp(c,w);assert(t.available&&t.volume==0&&t.cloud.points.empty());w.level=-2;t=virtualTarp(c,w);assert(t.volume==30);
 Comparison comparison;comparison.options.step=10;comparison.cells[{0,0}]={5.00000001,0,5.00000001,6,false};w.level=3;t=virtualTarp(c,w,&comparison);assert(t.volume==(5.00000001-3)*6);
 std::cout<<"PASS rim clipping, zero excess, unknown level, double-precision cells, original unchanged\n";}
