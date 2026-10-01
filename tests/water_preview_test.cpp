#include "../src/water_preview.hpp"
#include <cassert>
#include <iostream>
int main(){
    WaterFill w;w.available=true;w.level=200;w.volume=1234;
    auto cell=[&](int x,int y){float a=x*100,b=y*100;for(Point p:std::initializer_list<Point>{{a,b,0},{a+100,b,0},{a+100,b+100,0},{a,b+100,0},{a,b,200},{a+100,b,200},{a+100,b+100,200},{a,b+100,200}})w.geometry.points.push_back(p);};
    for(int y=0;y<3;y++)for(int x=0;x<3;x++)if(x!=1||y!=1)cell(x,y);
    auto before=w.geometry.points.size();auto m=waterPreviewSurface(w,100);assert(m.faces.size()==18&&m.points.size()==36);for(auto p:m.points)assert(p.z==200);assert(w.volume==1234&&w.geometry.points.size()==before);
    w.geometry.points.clear();for(int y=0;y<3;y++)for(int x=0;x<3;x++)if(x!=1||y==2)cell(x,y);
    m=waterPreviewSurface(w,100);assert(m.faces.size()==14); // An open boundary is not closed by the preview.
    assert(waterPreviewSurface(w,0).points.empty());w.available=false;assert(waterPreviewSurface(w,100).points.empty());
    std::cout<<"PASS continuous level, enclosed display hole, open boundary, immutable volume and invalid inputs\n";
}
