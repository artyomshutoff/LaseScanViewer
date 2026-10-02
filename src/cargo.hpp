#pragma once
#include "analysis.hpp"
#include "body_filter.hpp"
#include <memory>
#include <set>
struct Cargo {Model geometry,cloud;double volume=0,reconstructedVolume=0;size_t cells=0;double threshold=0;bool largest=true,cleaned=false;size_t removedCells=0,bodyPointsExcluded=0;bool calibrated=false;double calibratedVolume=0;std::string calibrationNote;};
// Diagnostic variants need the exact integral and mask, without allocating
// display meshes, clouds or body-observation indexes. The public default keeps
// all display geometry; both paths use the same cell selection and sum order.
inline Cargo buildCargo(const Comparison& c,double threshold,bool largest,const Model* active=nullptr,bool clean=false,const Model* baseline=nullptr,bool buildGeometry=true){
    if(!std::isfinite(threshold)||threshold<0)throw std::runtime_error("Invalid cargo height threshold");
    std::set<CellKey> pending;for(const auto& [k,v]:c.cells)if(v.delta>threshold)pending.insert(k);
    size_t rawCells=pending.size();
    if(clean){
        std::set<CellKey> eligible=pending;
        (void)baseline; // Low surface may be the bed floor, not road. Never remove it by height alone.
        // A baseline step can be an internal rib or a sloping floor. A large
        // baseline gradient alone is not evidence that the material is a wall.
        // Reject only a small height residual on a steep feature that exists
        // in BOTH scans. A substantial layer above a rib remains eligible.
        for(auto k:pending){const auto& v=c.cells.at(k);if(v.delta>2*threshold)continue;
            for(auto d:std::initializer_list<CellKey>{{-1,0},{1,0},{0,-1},{0,1}}){
                auto it=c.cells.find({k.first+d.first,k.second+d.second});if(it==c.cells.end())continue;
                double db=it->second.base-v.base,da=it->second.active-v.active;
                if(std::abs(db)>1.5*c.options.step&&std::abs(da-db)<.25*std::abs(db)){eligible.erase(k);break;}
            }
        }
        // Keep broad supported regions; do not flood back along thin connected rims.
        std::set<CellKey> core,mask;int radius=std::clamp(int(std::ceil(.025*std::min(c.region.x1-c.region.x0,c.region.y1-c.region.y0)/c.options.step)),1,4);int required=int(std::ceil(.72*(2*radius+1)*(2*radius+1)));
        for(auto k:eligible){if(c.cells.at(k).delta<=2*threshold)continue;int support=0;for(int x=-radius;x<=radius;x++)for(int y=-radius;y<=radius;y++)support+=int(eligible.count({k.first+x,k.second+y}));if(support>=required)core.insert(k);}
        for(auto k:core)for(int x=-radius;x<=radius;x++)for(int y=-radius;y<=radius;y++){CellKey q{k.first+x,k.second+y};if(eligible.count(q))mask.insert(q);}
        pending=std::move(mask);
    }
    size_t filteredCells=pending.size();
    std::vector<CellKey> chosen;
    double best=-1;
    while(!pending.empty()){
        std::vector<CellKey> component{*pending.begin()};pending.erase(pending.begin());double volume=0;
        for(size_t i=0;i<component.size();i++){auto k=component[i];auto& cell=c.cells.at(k);volume+=cell.delta*cell.area;
            for(int x=-1;x<=1;x++)for(int y=-1;y<=1;y++){auto it=pending.find({k.first+x,k.second+y});if(it!=pending.end()){component.push_back(*it);pending.erase(it);}}
        }
        if(!largest)chosen.insert(chosen.end(),component.begin(),component.end());else if(volume>best){best=volume;chosen=std::move(component);}
    }
    Cargo out;out.threshold=threshold;out.largest=largest;out.cleaned=clean;out.removedCells=rawCells-filteredCells;
    if(buildGeometry){out.geometry.points.reserve(chosen.size()*8);out.geometry.faces.reserve(chosen.size()*12);}
    // Each visible prism exactly represents one included integral cell.
    for(auto k:chosen){auto v=c.cells.at(k);float x=float(c.region.x0+k.first*c.options.step),y=float(c.region.y0+k.second*c.options.step),xx=float(std::min(c.region.x1,double(x)+c.options.step)),yy=float(std::min(c.region.y1,double(y)+c.options.step));float z=float(v.base),zz=float(v.active);uint32_t n=uint32_t(out.geometry.points.size());
        if(buildGeometry){for(Point p:std::initializer_list<Point>{{x,y,z},{xx,y,z},{xx,yy,z},{x,yy,z},{x,y,zz},{xx,y,zz},{xx,yy,zz},{x,yy,zz}})out.geometry.points.push_back(p);
        for(auto f:std::initializer_list<std::array<uint32_t,3>>{{0,2,1},{0,3,2},{4,5,6},{4,6,7},{0,1,5},{0,5,4},{1,2,6},{1,6,5},{2,3,7},{2,7,6},{3,0,4},{3,4,7}})out.geometry.faces.push_back({n+f[0],n+f[1],n+f[2]});}
        out.volume+=v.delta*v.area;if(v.reconstructed)out.reconstructedVolume+=v.delta*v.area;++out.cells;
    }
    if(!buildGeometry)return out;
    if(!out.geometry.points.empty()){auto& m=out.geometry;m.lo=m.hi=m.points.front();for(auto p:m.points){m.lo.x=std::min(m.lo.x,p.x);m.lo.y=std::min(m.lo.y,p.y);m.lo.z=std::min(m.lo.z,p.z);m.hi.x=std::max(m.hi.x,p.x);m.hi.y=std::max(m.hi.y,p.y);m.hi.z=std::max(m.hi.z,p.z);}}
    if(active){
        struct SurfaceLimits{double floor,lower,upper;};std::map<CellKey,SurfaceLimits> included;
        // The surface band depends on the cell, not the individual point.
        // Cache it once and retain exact inclusive comparisons for each point.
        for(auto k:chosen){const auto& cell=c.cells.at(k);double band=clean?cargoSurfaceBand(c,k,threshold):0;
            included.emplace(k,SurfaceLimits{cell.base+threshold,cell.active-band,cell.active+band});}
        std::unique_ptr<BodyObservations> body;
        if(clean&&baseline)body=std::make_unique<BodyObservations>(*baseline,c.options,std::max(threshold,c.options.step));
        for(auto p:active->points){
            if(!c.region.contains(p))continue;
            auto k=c.key(p);k.first=std::min(k.first,int(std::ceil((c.region.x1-c.region.x0)/c.options.step))-1);k.second=std::min(k.second,int(std::ceil((c.region.y1-c.region.y0)/c.options.step))-1);
            auto found=included.find(k);if(found==included.end()||p.z<=found->second.floor)continue;
            if(clean&&(!(p.z>=found->second.lower&&p.z<=found->second.upper)||(body&&body->matches(p)))){++out.bodyPointsExcluded;continue;}
            out.cloud.points.push_back(p);
        }
    }else out.cloud.points=out.geometry.points;
    out.cloud.lo=out.geometry.lo;out.cloud.hi=out.geometry.hi;
    return out;
}
