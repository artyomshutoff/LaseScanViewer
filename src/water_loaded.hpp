#pragma once
#include "water_fill.hpp"
// Remaining space above the measured FULL surface within the EMPTY basin.
// Unknown FULL cells are left open; cargo above the spill level adds no water.
inline WaterFill waterAboveLoad(const Model& full,const Comparison& comparison,const WaterFill& capacity){
    WaterFill out;out.level=capacity.level;
    if(!capacity.available){out.note="Для воды над грузом нужны замкнутые борта Empty";return out;}
    double step=comparison.requestedStep>0?comparison.requestedStep:comparison.options.step;
    std::map<CellKey,HeightCell> surface;
    for(auto p:full.points)if(comparison.region.contains(p))surface[{int(std::floor(p.x/step)),int(std::floor(p.y/step))}].add(p.z);
    for(size_t i=0;i+7<capacity.geometry.points.size();i+=8){auto p=capacity.geometry.points[i];CellKey k{int(std::llround(p.x/step)),int(std::llround(p.y/step))};auto found=surface.find(k);if(found==surface.end())continue;
        // Use the upper surface so filling cannot pass through measured cargo.
        double bottom=std::max(double(p.z),found->second.value(0));if(bottom>=out.level)continue;
        uint32_t first=uint32_t(out.geometry.points.size());
        for(size_t j=0;j<8;j++){auto q=capacity.geometry.points[i+j];q.z=float(j<4?bottom:out.level);out.geometry.points.push_back(q);}
        out.geometry.faces.push_back({first+4,first+5,first+6});out.geometry.faces.push_back({first+4,first+6,first+7});
        out.volume+=(out.level-bottom)*step*step;out.area+=step*step;++out.cells;
    }
    out.available=true;out.note="Свободное пространство над измеренной поверхностью Full до уровня бортов Empty; неизвестные ячейки исключены";
    if(!out.geometry.points.empty()){out.geometry.lo=out.geometry.hi=out.geometry.points.front();for(auto p:out.geometry.points){auto& m=out.geometry;m.lo.x=std::min(m.lo.x,p.x);m.lo.y=std::min(m.lo.y,p.y);m.lo.z=std::min(m.lo.z,p.z);m.hi.x=std::max(m.hi.x,p.x);m.hi.y=std::max(m.hi.y,p.y);m.hi.z=std::max(m.hi.z,p.z);}}
    return out;
}
