#pragma once
#include "cargo.hpp"

struct BedAnchor {
    double shift=0,mad=0,spacing=0,coverageX=0,coverageY=0;
    size_t count=0;
};
inline double anchorMedian(std::vector<double> values){
    if(values.empty())return 0;
    auto mid=values.begin()+values.size()/2;
    std::nth_element(values.begin(),mid,values.end());return *mid;
}
// Restrict anchors to the loaded bed and its immediate boundary. The cab,
// road and distant platforms must not determine the bed's vertical datum.
// Order: band (whole bed/boundary), scale, upper cutoff, median/top return.
inline std::array<BedAnchor,36> bedAnchors(const Model& active,const Model& base,
                                         const Comparison& c,const Cargo& cargo){
    std::array<BedAnchor,36> result{};
    if(cargo.geometry.points.empty())return result;
    double span=std::max(active.hi.x-active.lo.x,active.hi.y-active.lo.y);
    if(span<=0)return result;
    std::set<CellKey> inside,expanded,eroded;
    for(size_t i=0;i<cargo.geometry.points.size();i+=8){
        auto p=cargo.geometry.points[i];
        inside.insert({int(std::llround((p.x-c.region.x0)/c.options.step)),
                       int(std::llround((p.y-c.region.y0)/c.options.step))});
    }
    for(auto k:inside){
        bool surrounded=true;
        for(int x=-2;x<=2;++x)for(int y=-2;y<=2;++y){
            CellKey q{k.first+x,k.second+y};expanded.insert(q);
            if(!inside.count(q))surrounded=false;
        }
        if(surrounded)eroded.insert(k);
    }
    size_t index=0;
    for(int band:{0,1})for(double divisor:{60.,100.,160.}){
        double step=span/divisor;
        std::map<CellKey,std::vector<double>> a,b;std::vector<double> heights;
        auto allowed=[&](Point p){auto k=c.key(p);return expanded.count(k)&&(!band||!eroded.count(k));};
        for(auto p:active.points)if(allowed(p))
            a[{int(std::floor(p.x/step)),int(std::floor(p.y/step))}].push_back(p.z);
        for(auto p:base.points){p=transformBase(p,c.options);if(allowed(p)){
            b[{int(std::floor(p.x/step)),int(std::floor(p.y/step))}].push_back(p.z);heights.push_back(p.z);
        }}
        for(auto& kv:a)std::sort(kv.second.begin(),kv.second.end());
        for(auto& kv:b)std::sort(kv.second.begin(),kv.second.end());
        std::sort(heights.begin(),heights.end());
        for(double cut:{.8,.9,.95})for(double quantile:{.5,1.}){
            auto& anchor=result[index++];anchor.spacing=step;
            if(heights.empty())continue;
            double lower=heights[size_t((heights.size()-1)*cut)]-step;
            std::vector<double> shifts;double xmin=1e30,xmax=-1e30,ymin=1e30,ymax=-1e30;
            for(const auto&[k,v]:b){
                auto it=a.find(k);
                if(v.size()<3||it==a.end()||it->second.size()<3||v.back()<lower)continue;
                const auto& w=it->second;
                double d=w[size_t((w.size()-1)*quantile)]-v[size_t((v.size()-1)*quantile)];
                if(std::abs(d)>=3*step)continue;
                shifts.push_back(d);
                xmin=std::min(xmin,k.first*step);xmax=std::max(xmax,k.first*step);
                ymin=std::min(ymin,k.second*step);ymax=std::max(ymax,k.second*step);
            }
            anchor.count=shifts.size();anchor.shift=anchorMedian(shifts);
            for(auto& s:shifts)s=std::abs(s-anchor.shift);
            anchor.mad=anchorMedian(shifts);
            if(anchor.count){
                anchor.coverageX=(xmax-xmin)/std::max(1.,double(cargo.geometry.hi.x-cargo.geometry.lo.x));
                anchor.coverageY=(ymax-ymin)/std::max(1.,double(cargo.geometry.hi.y-cargo.geometry.lo.y));
            }
        }
    }
    return result;
}
struct BedCorrection {double shift=0,spread=0;size_t variants=0;};
inline BedCorrection bedHeightCorrection(const std::array<BedAnchor,36>& anchors,double span){
    BedCorrection result;double sum2=0,weight=0;
    if(!std::isfinite(span)||span<=0)return {};
    // Normalize residual dispersion by anchor spacing. Prefer internally
    // consistent hypotheses without letting densely sampled scans dominate.
    for(size_t i=1;i<18;i+=2){const auto& a=anchors[i];
        if(a.count<24||!std::isfinite(a.shift)||!std::isfinite(a.mad)||
           !std::isfinite(a.spacing)||a.spacing<=0||a.mad<0||
           a.mad>1.5*a.spacing||a.coverageX<.6||a.coverageY<.6)continue;
        double w=1/(1+a.mad/a.spacing);
        result.shift+=w*a.shift;sum2+=w*a.shift*a.shift;weight+=w;++result.variants;
    }
    if(result.variants<6)return {};
    result.shift/=weight;
    result.spread=std::sqrt(std::max(0.,sum2/weight-result.shift*result.shift));
    if(!std::isfinite(result.shift)||std::abs(result.shift)>span*.01||result.spread>span*.0075)return {};
    return result;
}

// Called once after global registration, never on an already corrected grid.
// Reference scans and sparse/unsupported beds keep their original rigid pose.
inline CompareOptions refineBedHeight(const Model& active,const Model& base,CompareOptions options){
    options.bedHeightAdjustment=options.bedHeightSpread=0;options.bedHeightVariants=0;
    if(active.kind!=2||base.kind!=1||active.selectedType!=0||base.selectedType!=0||options.estimator!=4)return options;
    auto probe=options;probe.aligned=true;
    try{
        auto c=compareClouds(active,base,{},probe);auto cargo=buildCargo(c,50,true,nullptr,true);
        if(options.adaptiveGrid&&active.points.size()>=10*c.activeCells&&base.points.size()>=10*c.baseCells){
            probe.step=options.step*.75;c=compareClouds(active,base,{},probe);cargo=buildCargo(c,50,true,nullptr,true);
            if(cargo.volume>0&&cargo.reconstructedVolume>cargo.volume*.01){
                probe.step=options.step*1.25;c=compareClouds(active,base,{},probe);cargo=buildCargo(c,50,true,nullptr,true);
            }
        }
        auto correction=bedHeightCorrection(bedAnchors(active,base,c,cargo),
                                            std::max(active.hi.x-active.lo.x,active.hi.y-active.lo.y));
        options.bedHeightAdjustment=correction.shift;options.bedHeightSpread=correction.spread;
        options.bedHeightVariants=correction.variants;options.dz+=correction.shift;
    }catch(const std::runtime_error&){
        // Insufficient common support is not permission to invent a datum.
    }
    return options;
}
