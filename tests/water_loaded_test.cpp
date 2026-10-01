#include "../src/water_loaded.hpp"
#include <cassert>
#include <cmath>
#include <iostream>
int main(){
    Comparison c;c.requestedStep=100;c.options.step=100;
    WaterFill capacity;capacity.available=true;capacity.level=200;
    for(int x=0;x<3;x++){float a=float(x*100);for(Point p:std::initializer_list<Point>{{a,0,0},{a+100,0,0},{a+100,100,0},{a,100,0},{a,0,200},{a+100,0,200},{a+100,100,200},{a,100,200}})capacity.geometry.points.push_back(p);}
    Model full;full.points={{50,50,50},{150,50,100},{250,50,250}};
    auto w=waterAboveLoad(full,c,capacity);assert(w.available&&w.cells==2);assert(std::abs(w.volume-2500000)<.01);assert(w.level==200);assert(w.geometry.faces.size()==4);
    full.points.push_back({50,50,175});w=waterAboveLoad(full,c,capacity);assert(std::abs(w.volume-1250000)<.01); // Upper samples prevent filling through cargo.
    full.points.clear();w=waterAboveLoad(full,c,capacity);assert(w.available&&w.cells==0&&w.volume==0); // Unknown cells never fabricated.
    full.points={{50,50,300}};w=waterAboveLoad(full,c,capacity);assert(w.available&&w.cells==0&&w.volume==0);
    capacity.available=false;assert(!waterAboveLoad(full,c,capacity).available);
    std::cout<<"PASS measured free space, upper surface, missing cells, heap above rim, unavailable basin\n";
}
