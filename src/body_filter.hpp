#pragma once
#include "analysis.hpp"

// Match actual 3D observations of the aligned empty body, not merely its
// lowest height in an XY cell. The latter loses tall walls in mixed cells.
class BodyObservations {
    double radius;
    std::map<CellKey,std::vector<Point>> bins;
public:
    BodyObservations(const Model& base,const CompareOptions& options,double tolerance):radius(tolerance){
        for(auto p:base.points){p=transformBase(p,options);bins[{int(std::floor(p.x/radius)),int(std::floor(p.y/radius))}].push_back(p);}
        for(auto& [key,points]:bins){(void)key;std::sort(points.begin(),points.end(),[](Point a,Point b){return a.z<b.z;});}
    }
    bool matches(Point p)const{
        CellKey key{int(std::floor(p.x/radius)),int(std::floor(p.y/radius))};
        for(int x=-1;x<=1;x++)for(int y=-1;y<=1;y++){
            auto it=bins.find({key.first+x,key.second+y});if(it==bins.end())continue;
            const auto& points=it->second;
            auto start=std::lower_bound(points.begin(),points.end(),p.z-radius,[](Point q,double z){return q.z<z;});
            for(auto q=start;q!=points.end()&&q->z<=p.z+radius;++q){
                double dx=p.x-q->x,dy=p.y-q->y,dz=p.z-q->z;
                if(dx*dx+dy*dy+dz*dz<=radius*radius)return true;
            }
        }
        return false;
    }
};

inline bool nearCargoSurface(const Comparison& c,CellKey k,Point p,double threshold){
    const auto& cell=c.cells.at(k);
    // The quantile describes the material surface. Do not color the entire
    // vertical column, which also contains the exterior body and tall rails.
    double slope=0;
    for(auto d:std::initializer_list<CellKey>{{1,0},{-1,0},{0,1},{0,-1}}){
        auto it=c.cells.find({k.first+d.first,k.second+d.second});
        if(it!=c.cells.end()&&it->second.delta>threshold)
            slope=std::max(slope,std::abs(it->second.active-cell.active));
    }
    double band=std::max(threshold,c.options.step*.75)+std::min(slope,c.options.step)*.5;
    return p.z>=cell.active-band&&p.z<=cell.active+band;
}
