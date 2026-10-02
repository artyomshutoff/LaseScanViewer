#pragma once
#include "cargo.hpp"
#include "calibration.hpp"
#include <functional>
#include "parallel_work.hpp"

struct VolumeResult {Comparison comparison;Cargo cargo;};
// A finer grid can split a supported cargo component at sparse scan seams.
// Do not replace the requested integral with that fragment when two coarser
// resolutions agree. Keep the requested grid, never the largest answer, and
// mark the result provisional. This is support evidence, not a target fit.
inline bool rejectFragmentedRefinement(double requested,double fine,double supported){
    return std::isfinite(requested)&&std::isfinite(fine)&&std::isfinite(supported)&&
        requested>0&&fine>=0&&supported>0&&fine<requested*.75&&
        std::abs(supported-requested)<=requested*.25;
}
// Resolve dense surfaces on a fine grid; use a coarser supported surface when
// occlusions contribute appreciably. This rule uses geometry, not target volumes.
inline VolumeResult calculateVolume(const Model& active,const Model& base,Region region,
    CompareOptions options,double threshold,bool largest,bool clean,
    const std::function<void(int)>& progress={}){
    auto notify=[&](int n){if(progress)progress(n);};notify(0);
    VolumeResult out;out.comparison=compareClouds(active,base,region,options);notify(15);
    out.cargo=buildCargo(out.comparison,threshold,largest,&active,clean,&base);notify(20);
    std::map<double,double> resolvedVolumes{{1.,out.cargo.volume}};
    bool adaptive=options.adaptiveGrid&&options.estimator==4&&
        active.points.size()>=10*out.comparison.activeCells&&base.points.size()>=10*out.comparison.baseCells;
    bool refinementRejected=false;
    if(adaptive){
        auto fine=options;fine.step*=.75;
        auto grid=compareClouds(active,base,region,fine);
        auto material=buildCargo(grid,threshold,largest,&active,clean,&base);notify(25);
        resolvedVolumes[.75]=material.volume;
        bool lostSupport=out.cargo.volume>0&&material.volume<out.cargo.volume*.75;
        bool needsCoarse=material.volume>0&&material.reconstructedVolume>material.volume*.01;
        if(lostSupport||needsCoarse){
            auto supported=options;supported.step*=1.25;
            auto coarseGrid=compareClouds(active,base,region,supported);
            auto coarseMaterial=buildCargo(coarseGrid,threshold,largest,&active,clean,&base);
            resolvedVolumes[1.25]=coarseMaterial.volume;
            refinementRejected=rejectFragmentedRefinement(out.cargo.volume,material.volume,coarseMaterial.volume);
            if(!refinementRejected&&needsCoarse){grid=std::move(coarseGrid);material=std::move(coarseMaterial);}
        }
        if(!refinementRejected){out.comparison=std::move(grid);out.cargo=std::move(material);}
    }
    out.comparison.requestedStep=options.step;out.comparison.adaptiveGridUsed=adaptive;out.comparison.refinementRejected=refinementRejected;notify(30);
    auto& c=out.comparison;auto& cargo=out.cargo;
    if(refinementRejected){c.options.provisional=true;c.warning+=" Мелкая сетка раздробила область груза: сохранён исходный шаг, подтверждённый более крупной сеткой. Проверьте пропуски и совмещение.";}
    if(c.negative>std::max(c.positive*.3,c.sharedArea*threshold*.5)){
        c.options.provisional=true;c.warning+=" Значительная часть Full ниже Empty: проверьте совмещение и границу кузова.";
    }
    c.sensitivityMin=c.sensitivityMax=cargo.volume;c.sensitivityChecked=true;
    std::array<double,5> factors{.5,.75,1.,1.25,1.5};
    struct Check {double volume=0;bool valid=false;};std::array<Check,8> checks;
    size_t count=region.enabled?5:8;
    // Grid placement can change which wall and floor returns share a cell.
    // Diagnose this independently of resolution, without selecting the largest
    // answer. Padding is used only for automatic bounds; explicit ROI is fixed.
    parallelJobs(count,[&](size_t index){try{
        if(index<5){auto known=resolvedVolumes.find(factors[index]);if(known!=resolvedVolumes.end()){checks[index]={known->second,true};return;}
            auto variant=options;variant.step*=factors[index];auto grid=compareClouds(active,base,region,variant);
            checks[index]={buildCargo(grid,threshold,largest,nullptr,clean,nullptr,false).volume,true};
        }else{int phase=int(index)-4;auto shifted=c.region;shifted.x0-=(phase&1)?c.options.step*.5:0;shifted.y0-=(phase&2)?c.options.step*.5:0;
            auto grid=compareClouds(active,base,shifted,c.options);checks[index]={buildCargo(grid,threshold,largest,nullptr,clean,nullptr,false).volume,true};}
    }catch(const std::exception&){checks[index].valid=false;}});
    for(size_t index=0;index<count;index++){auto check=checks[index];if(check.valid){c.sensitivityMin=std::min(c.sensitivityMin,check.volume);c.sensitivityMax=std::max(c.sensitivityMax,check.volume);}else{c.sensitivityChecked=false;c.options.provisional=true;}
        notify(index<5?30+int(index+1)*8:70+int(index-4)*8);
    }
    if(!c.sensitivityChecked)c.warning+=" Проверка некоторых разрешений недоступна.";
    if(c.sensitivityChecked&&c.sensitivityMax-c.sensitivityMin>std::max(cargo.volume*.25,1.)){
        c.options.provisional=true;c.warning+=" Результат заметно зависит от шага сетки.";
    }
    if(cargo.reconstructedVolume>cargo.volume*.1){
        c.options.provisional=true;c.warning+=" Более 10% объёма относится к восстановленным ячейкам.";
    }
    // Propagate disagreement between bed-height hypotheses into an explicit
    // diagnostic range. This is sensitivity, not a statistical confidence bound.
    if(!region.enabled&&options.bedHeightVariants>=6&&options.bedHeightSpread>0){
        c.datumSensitivityChecked=true;c.datumSensitivityMin=c.datumSensitivityMax=cargo.volume;
        for(double sign:{-1.,1.}){
            auto variant=c;double shift=sign*options.bedHeightSpread;
            for(auto& kv:variant.cells){kv.second.base+=shift;kv.second.delta-=shift;}
            double volume=buildCargo(variant,threshold,largest,nullptr,clean,nullptr,false).volume;
            c.datumSensitivityMin=std::min(c.datumSensitivityMin,volume);
            c.datumSensitivityMax=std::max(c.datumSensitivityMax,volume);
        }
        if(c.datumSensitivityMax-c.datumSensitivityMin>std::max(1.,cargo.volume*.05)){
            c.options.provisional=true;c.warning+=" Объём чувствителен к выбору опорной высоты кузова.";
        }
    }
    notify(95);
    calibrateVolume(active,base,c,cargo,region.enabled,[&](int n){notify(95+n*4/94);});
    notify(100);return out;
}
