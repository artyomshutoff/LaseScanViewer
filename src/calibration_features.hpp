#pragma once
#include "bed_alignment.hpp"
#include "cargo_shape.hpp"
#include <functional>
#include "parallel_work.hpp"
// Geometric descriptors only. No model coefficients, targets or metadata.
inline std::array<double,190> volumeFeatures(const Model& active,const Model& base,
    const Comparison& c,const Cargo& cargo,const std::function<void(int)>& progress={}){
    double span=std::max(active.hi.x-active.lo.x,active.hi.y-active.lo.y);
    if(!std::isfinite(span)||span<=0||cargo.volume<=0)return {};
    std::vector<double> allZ;allZ.reserve(base.points.size());
    for(auto p:base.points)allZ.push_back(p.z+c.options.dz);
    std::sort(allZ.begin(),allZ.end());
    if(allZ.empty())return {};
    // The same height shift is less informative when supported by few anchors
    // or a dispersed residual distribution. Preserve both diagnostics instead
    // of treating all 48 alternative reference surfaces as equally reliable.
    std::array<double,190> features{};
    auto median=[](std::vector<double> values){
        if(values.empty())return 0.;
        auto mid=values.begin()+values.size()/2;
        std::nth_element(values.begin(),mid,values.end());return *mid;
    };
    PreparedBaseTransform prepared(c.options);
    const std::array<double,4> divisors{60.,100.,160.,250.};
    parallelJobs(divisors.size(),[&](size_t block){
        double divisor=divisors[block];size_t index=block*12;
        double step=span/divisor;std::map<CellKey,std::vector<double>> a,b;
        for(auto p:active.points)a[{int(std::floor(p.x/step)),int(std::floor(p.y/step))}].push_back(p.z);
        for(auto p:base.points){p=prepared(p);b[{int(std::floor(p.x/step)),int(std::floor(p.y/step))}].push_back(p.z);}
        for(auto&[k,v]:a){(void)k;std::sort(v.begin(),v.end());}
        for(auto&[k,v]:b){(void)k;std::sort(v.begin(),v.end());}
        for(double cut:{.80,.90,.95,.98})for(double quantile:{.5,.9,1.}){
            double lower=allZ[size_t((allZ.size()-1)*cut)]-step;std::vector<double> shifts;
            for(const auto&[k,v]:b){
                auto it=a.find(k);if(v.size()<3||it==a.end()||it->second.size()<3||v.back()<lower)continue;
                const auto&w=it->second;double d=w[size_t((w.size()-1)*quantile)]-v[size_t((v.size()-1)*quantile)];
                if(std::abs(d)<3*step)shifts.push_back(d);
            }
            double shift=median(shifts);
            std::vector<double> deviations;deviations.reserve(shifts.size());
            for(double d:shifts)deviations.push_back(std::abs(d-shift));
            auto variant=c;
            for(auto&[k,v]:variant.cells){(void)k;v.base+=shift;v.delta-=shift;}
            double candidate=buildCargo(variant,cargo.threshold,true,nullptr,true).volume;
            features[index]=candidate/cargo.volume-1;
            features[48+index]=median(deviations)/1000.;
            features[96+index]=std::log1p(double(shifts.size()));
            ++index;
        }
    });if(progress)progress(48);
    auto anchors=bedAnchors(active,base,c,cargo);
    for(size_t i=0;i<anchors.size();++i){
        auto variant=c;
        for(auto& kv:variant.cells){kv.second.base+=anchors[i].shift;kv.second.delta-=anchors[i].shift;}
        features[144+i]=buildCargo(variant,cargo.threshold,true,nullptr,true).volume/cargo.volume-1;
        if(progress)progress(49+int(i));
    }
    auto shape=cargoShapeFeatures(c,cargo);
    for(size_t i=0;i<shape.size();++i){features[180+i]=shape[i];if(progress)progress(85+int(i));}
    return features;
}
