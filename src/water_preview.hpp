#pragma once
#include "water_fill.hpp"
// A continuous reference plane for display only. Filling enclosed holes here
// never changes capacity or measured remaining-water volume.
inline Model waterPreviewSurface(const WaterFill& capacity,double step){
    Model m;if(!capacity.available||capacity.geometry.points.empty()||!std::isfinite(step)||step<=0)return m;
    std::set<CellKey> cells;
    for(size_t i=0;i+7<capacity.geometry.points.size();i+=8){auto p=capacity.geometry.points[i];cells.insert({int(std::llround(p.x/step)),int(std::llround(p.y/step))});}
    int x0=cells.begin()->first,x1=x0,y0=cells.begin()->second,y1=y0;
    for(auto [x,y]:cells){x0=std::min(x0,x);x1=std::max(x1,x);y0=std::min(y0,y);y1=std::max(y1,y);}
    int nx=x1-x0+3,ny=y1-y0+3;
    if(int64_t(nx)*ny<=1000000){
        std::vector<bool> exterior(size_t(nx)*ny);std::queue<CellKey> q;q.push({x0-1,y0-1});
        auto index=[&](int x,int y){return size_t(y-y0+1)*nx+x-x0+1;};exterior[0]=true;
        while(!q.empty()){auto [x,y]=q.front();q.pop();for(auto [dx,dy]:std::array<CellKey,4>{{{1,0},{-1,0},{0,1},{0,-1}}}){int a=x+dx,b=y+dy;if(a<x0-1||a>x1+1||b<y0-1||b>y1+1||cells.count({a,b}))continue;auto i=index(a,b);if(!exterior[i]){exterior[i]=true;q.push({a,b});}}}
        for(int y=y0;y<=y1;y++)for(int x=x0;x<=x1;x++)if(!exterior[index(x,y)])cells.insert({x,y});
    }
    for(auto [x,y]:cells){uint32_t first=uint32_t(m.points.size());float a=float(x*step),b=float(y*step),z=float(capacity.level);
        for(Point p:std::initializer_list<Point>{{a,b,z},{float(a+step),b,z},{float(a+step),float(b+step),z},{a,float(b+step),z}})m.points.push_back(p);
        m.faces.push_back({first,first+1,first+2});m.faces.push_back({first,first+2,first+3});}
    return m;
}
