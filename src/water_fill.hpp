#pragma once
#include "cargo.hpp"
#include <queue>
struct WaterFill {
    Model geometry;
    double volume=0,level=0,area=0;
    size_t cells=0,interpolated=0;
    bool available=false;
    std::string note;
};
// A hydraulic depression fill of the measured EMPTY surface. The minimum
// escape path through its walls determines the spill level, not the cargo top.
// Missing surfaces drain the basin; only short, bracketed sampling gaps close.
inline WaterFill waterFill(const Model& empty,const Comparison& comparison,const Cargo& cargo){
    WaterFill out;double step=comparison.requestedStep>0?comparison.requestedStep:comparison.options.step;
    if(empty.points.empty()||!std::isfinite(step)||step<=0){out.note="Нет поверхности пустого кузова";return out;}
    std::map<CellKey,HeightCell> sampled;
    for(auto p:empty.points){p=transformBase(p,comparison.options);if(!comparison.region.contains(p))continue;
        sampled[{int(std::floor(p.x/step)),int(std::floor(p.y/step))}].add(p.z);}
    if(sampled.empty()){out.note="Нет Empty в области расчёта";return out;}
    int xmin=sampled.begin()->first.first,xmax=xmin,ymin=sampled.begin()->first.second,ymax=ymin;
    for(auto& kv:sampled){xmin=std::min(xmin,kv.first.first);xmax=std::max(xmax,kv.first.first);ymin=std::min(ymin,kv.first.second);ymax=std::max(ymax,kv.first.second);}
    int nx=xmax-xmin+1,ny=ymax-ymin+1;
    if(nx<3||ny<3||int64_t(nx)*ny>1000000){out.note="Недостаточная или слишком большая сетка Empty";return out;}
    const double missing=std::numeric_limits<double>::quiet_NaN();
    std::vector<double> height(size_t(nx)*ny,missing),floor(height),level(height);
    std::vector<bool> filled(height.size(),false),seen(height.size(),false);
    auto index=[&](int x,int y){return size_t(y)*nx+x;};
    for(auto& [k,v]:sampled){auto n=index(k.first-xmin,k.second-ymin);
        floor[n]=v.value(4);
        // Vertical wall cells retain their measured upper rim. Flat bed cells
        // keep the robust floor estimator used for the geometric volume.
        height[n]=v.hi-v.lo>step? v.value(0):floor[n];
    }
    const std::array<CellKey,4> directions={CellKey{-1,0},{1,0},{0,-1},{0,1}};
    auto original=height,originalFloor=floor;
    for(int y=1;y<ny-1;y++)for(int x=1;x<nx-1;x++)if(!std::isfinite(original[index(x,y)])){
        std::vector<double> candidates,bottoms;
        for(auto axis:{CellKey{1,0},CellKey{0,1}}){int left=0,right=0;
            for(int d=1;d<=3;d++){int xx=x-axis.first*d,yy=y-axis.second*d;if(xx<0||yy<0)break;if(std::isfinite(original[index(xx,yy)])){left=d;break;}}
            for(int d=1;d<=3;d++){int xx=x+axis.first*d,yy=y+axis.second*d;if(xx>=nx||yy>=ny)break;if(std::isfinite(original[index(xx,yy)])){right=d;break;}}
            if(!left||!right||left+right>4)continue;
            auto a=index(x-axis.first*left,y-axis.second*left),b=index(x+axis.first*right,y+axis.second*right);
            if(std::abs(original[a]-original[b])>step)continue;
            candidates.push_back((original[a]*right+original[b]*left)/(left+right));
            bottoms.push_back((originalFloor[a]*right+originalFloor[b]*left)/(left+right));
        }
        if(candidates.size()==2&&std::abs(candidates[0]-candidates[1])<=step){auto n=index(x,y);height[n]=(candidates[0]+candidates[1])/2;floor[n]=(bottoms[0]+bottoms[1])/2;filled[n]=true;}
    }
    using Entry=std::pair<double,size_t>;std::priority_queue<Entry,std::vector<Entry>,std::greater<Entry>> queue;
    for(int y=0;y<ny;y++)for(int x=0;x<nx;x++){auto n=index(x,y);if(!std::isfinite(height[n]))continue;
        bool boundary=x==0||y==0||x==nx-1||y==ny-1;
        for(auto d:directions){int xx=x+d.first,yy=y+d.second;if(xx>=0&&xx<nx&&yy>=0&&yy<ny&&!std::isfinite(height[index(xx,yy)]))boundary=true;}
        if(boundary){seen[n]=true;level[n]=height[n];queue.push({level[n],n});}
    }
    while(!queue.empty()){auto [water,n]=queue.top();queue.pop();int x=int(n%nx),y=int(n/nx);
        for(auto d:directions){int xx=x+d.first,yy=y+d.second;if(xx<0||yy<0||xx>=nx||yy>=ny)continue;auto q=index(xx,yy);if(seen[q]||!std::isfinite(height[q]))continue;
            seen[q]=true;level[q]=std::max(water,height[q]);queue.push({level[q],q});}
    }
    std::set<CellKey> cargoCells;
    for(size_t i=0;i<cargo.geometry.points.size();i+=8){auto p=cargo.geometry.points[i];cargoCells.insert({int(std::floor((p.x+comparison.options.step*.5)/step)),int(std::floor((p.y+comparison.options.step*.5)/step))});}
    std::vector<bool> pending(height.size());for(size_t n=0;n<height.size();n++)pending[n]=seen[n]&&level[n]-height[n]>std::max(1.,step*.25);
    std::vector<size_t> chosen;std::vector<std::vector<size_t>> supported;size_t bestOverlap=0;double bestVolume=0;
    for(size_t n=0;n<height.size();n++)if(pending[n]){
        std::vector<size_t> component{n};pending[n]=false;size_t overlap=0;double volume=0;
        for(size_t i=0;i<component.size();i++){auto q=component[i];int x=int(q%nx),y=int(q/nx);volume+=(level[q]-floor[q])*step*step;overlap+=cargoCells.count({x+xmin,y+ymin});
            for(auto d:directions){int xx=x+d.first,yy=y+d.second;if(xx<0||yy<0||xx>=nx||yy>=ny)continue;auto next=index(xx,yy);if(pending[next]){pending[next]=false;component.push_back(next);}}
        }
        if(component.size()>=9&&overlap>=5)supported.push_back(component);
        if(component.size()>=9&&(overlap>bestOverlap||(overlap==bestOverlap&&volume>bestVolume))){chosen=std::move(component);bestOverlap=overlap;bestVolume=volume;}
    }
    if(chosen.empty()||(!cargoCells.empty()&&bestOverlap==0)){out.note="Замкнутые борта не найдены: проверьте Empty, пропуски или область";return out;}
    out.level=level[chosen.front()];
    // Ribs and gaps can split one bed into several pools at the same spill
    // level. Include supported pools overlapping the same cargo region, using
    // the lowest measured spill level rather than adding incompatible levels.
    if(!supported.empty()){
        std::set<size_t> combined(chosen.begin(),chosen.end());double spill=out.level;
        for(const auto& pool:supported)if(std::abs(level[pool.front()]-out.level)<=step){spill=std::min(spill,level[pool.front()]);combined.insert(pool.begin(),pool.end());}
        chosen.assign(combined.begin(),combined.end());out.level=spill;
        chosen.erase(std::remove_if(chosen.begin(),chosen.end(),[&](size_t n){return out.level-height[n]<=step*.25;}),chosen.end());
    }
    std::set<size_t> interior(chosen.begin(),chosen.end());
    for(auto n:chosen){float x=float((int(n%nx)+xmin)*step),y=float((int(n/nx)+ymin)*step),z=float(floor[n]),top=float(out.level);auto& m=out.geometry;uint32_t first=uint32_t(m.points.size());
        for(Point p:std::initializer_list<Point>{{x,y,z},{float(x+step),y,z},{float(x+step),float(y+step),z},{x,float(y+step),z},{x,y,top},{float(x+step),y,top},{float(x+step),float(y+step),top},{x,float(y+step),top}})m.points.push_back(p);
        for(auto f:std::initializer_list<std::array<uint32_t,3>>{{4,5,6},{4,6,7}})m.faces.push_back({first+f[0],first+f[1],first+f[2]});
        const std::array<std::array<uint32_t,4>,4> sides={std::array<uint32_t,4>{3,0,4,7},{1,2,6,5},{0,1,5,4},{2,3,7,6}};
        for(size_t side=0;side<4;side++){int xx=int(n%nx)+directions[side].first,yy=int(n/nx)+directions[side].second;
            if(xx>=0&&xx<nx&&yy>=0&&yy<ny&&interior.count(index(xx,yy)))continue;auto f=sides[side];m.faces.push_back({first+f[0],first+f[1],first+f[2]});m.faces.push_back({first+f[0],first+f[2],first+f[3]});}
        out.volume+=(out.level-floor[n])*step*step;out.area+=step*step;out.interpolated+=filled[n];++out.cells;
    }
    out.available=true;out.note="Оценка по Empty до нижнего пути перелива через измеренные борта; крупные пропуски открыты";
    out.geometry.lo=out.geometry.hi=out.geometry.points.front();for(auto p:out.geometry.points){auto& m=out.geometry;m.lo.x=std::min(m.lo.x,p.x);m.lo.y=std::min(m.lo.y,p.y);m.lo.z=std::min(m.lo.z,p.z);m.hi.x=std::max(m.hi.x,p.x);m.hi.y=std::max(m.hi.y,p.y);m.hi.z=std::max(m.hi.z,p.z);}
    return out;
}
