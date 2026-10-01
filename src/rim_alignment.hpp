#pragma once
#include "analysis.hpp"
#include <climits>

// Independent height anchors on the empty body's upper structure. Changed
// material is excluded by spatial coverage, residual dispersion and trimming.
// No measurement IDs, database volumes or vehicle-specific offsets are used.
inline double rimHeightCorrection(const Model& active,const Model& base,const CompareOptions& options){
    double span=std::max({active.hi.x-active.lo.x,active.hi.y-active.lo.y,base.hi.x-base.lo.x,base.hi.y-base.lo.y});
    if(span<=0||base.points.empty())return 0;
    double cell=span/100;
    struct Top{double z=-1e30;size_t n=0;};std::map<CellKey,Top> a,b;std::vector<float> heights;
    for(auto p:active.points){auto& v=a[{int(std::floor(p.x/cell)),int(std::floor(p.y/cell))}];v.z=std::max(v.z,double(p.z));++v.n;}
    for(auto p:base.points){p=transformBase(p,options);auto& v=b[{int(std::floor(p.x/cell)),int(std::floor(p.y/cell))}];v.z=std::max(v.z,double(p.z));++v.n;heights.push_back(p.z);}
    auto quantile=heights.begin()+size_t((heights.size()-1)*.95);std::nth_element(heights.begin(),quantile,heights.end());double cut=*quantile-cell;
    std::vector<double> shifts;int xmin=INT_MAX,xmax=INT_MIN,ymin=INT_MAX,ymax=INT_MIN;
    int axmin=INT_MAX,axmax=INT_MIN,aymin=INT_MAX,aymax=INT_MIN;
    for(auto [k,v]:b){if(v.n<3||v.z<cut)continue;
        xmin=std::min(xmin,k.first);xmax=std::max(xmax,k.first);ymin=std::min(ymin,k.second);ymax=std::max(ymax,k.second);
        auto it=a.find(k);if(it==a.end()||it->second.n<3)continue;double dz=it->second.z-v.z;
        if(std::abs(dz)>3*cell)continue;
        shifts.push_back(dz);axmin=std::min(axmin,k.first);axmax=std::max(axmax,k.first);aymin=std::min(aymin,k.second);aymax=std::max(aymax,k.second);
    }
    if(shifts.size()<30||xmax<=xmin||ymax<=ymin||(axmax-axmin)<.65*(xmax-xmin)||(aymax-aymin)<.65*(ymax-ymin))return 0;
    auto median=[](std::vector<double> v){size_t n=v.size()/2;std::nth_element(v.begin(),v.begin()+n,v.end());return v[n];};
    double shift=median(shifts);for(auto& v:shifts)v=std::abs(v-shift);
    if(median(shifts)>cell*.75)return 0;
    return shift;
}
